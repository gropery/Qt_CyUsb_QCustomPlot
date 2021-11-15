/////////////////////////////////////////////////////////////////////////////
// USB以bulk形式传输,单次FPGA->PC上传1024*32bit数据,单次PC->FPGA下传1024*32bit数据
// PC->FPGA下传数据端点EP3OUT,非空标志flagb,表示当flagb=1时,FPGA可从USB中读出1024*32bit数据,faddr <= 2'b11;
// FPGA->PC上传数据端点EP0IN, 非满标志flaga,表示当flaga=1时,FPGA可向USB中写入1024*32bit数据,faddr <= 2'b00;
/////////////////////////////////////////////////////////////////////////////
module usb_controller(
	input clk,		//100MHz
	input rst_n,	
	//FX3 Slave FIFO接口
	input flaga,	//full flag EP0-IN
	input flagb,	//empty flag EP3-OUT
	input flagc,	//
	input flagd,	//
	//output fx3_pclk,		//Slave FIFO同步时钟信号
	output reg slcs,				//Slave FIFO片选信号，低电平有效
	output reg slwr,				//Slave FIFO写使能信号，低电平有效
	output reg slrd,				//Slave FIFO读使能信号，低电平有效
	output reg sloe,				//Slave FIFO输出使能信号，低电平有效
	output reg pktend,			//包结束信号
	output reg [1:0] faddr,		//操作FIFO地址
	inout[31:0] fdata,			//数据

	//引出端口，可使用signalTab调试
	output reg [63:0] tFpgaRead,
	output reg [63:0] tFpgaWaitWrite,
	output reg [63:0] tFpgaWrite,
	output reg [63:0] tPcLoop,
	output reg [63:0] tMaxPcLoop,
	output reg [7:0] state
);

//parameter declarations=====================================================//
localparam  
   IDLE      = 8'd0,
   PC2FPGA_0 = 8'd1,
   PC2FPGA_1 = 8'd2,
   PC2FPGA_2 = 8'd3,
   PC2FPGA_3 = 8'd4,
   PC2FPGA_4 = 8'd5,
   PC2FPGA_5 = 8'd6,
   FPGA2PC_0 = 8'd7,
   FPGA2PC_1 = 8'd8,
   FPGA2PC_2 = 8'd9,
   FPGA2PC_3 = 8'd10,
   FPGA2PC_4 = 8'd11,
   FPGA2PC_5 = 8'd12;

//interrnal wire/reg declarations============================================//
// 寄存器同步外部输入信号
reg flaga_r;
reg flagb_r;
reg flagc_r;
reg flagd_r;

reg usb_dir;						// 控制USB data端口input还是output
reg [31:0] tx_data;				// USB 待发送数据
reg [31:0] num;
reg [31:0] rx_data[1023:0];	// USB 接收到数据

reg [63:0] tPcToFpgaStart;		//USB下传端点有数据，FPGA读取数据开始时
reg [63:0] tPcToFpgaEnd;		//FPGA读取PC下发数据结束
reg [63:0] tFpgaToPcStart;		//USB上传端点可写入，FPGA写入数据开始时
reg [63:0] tFpgaToPcEnd;		//FPGA写入CP上传数据结束
reg catch;							//计算各个时间段的节点

//continuous assignment======================================================//
assign fdata = usb_dir? tx_data:32'dz;	

//always block===============================================================//
////---------------------------------------------------------------------------//
// 实现功能  : 同步USB Flag标识
//---------------------------------------------------------------------------//
always @(posedge clk, negedge rst_n)begin
	if(!rst_n)begin 
		flaga_r <= 1'd0;
		flagb_r <= 1'd0;
		flagc_r <= 1'd0;
		flagd_r <= 1'd0;
	end 
	else begin
		flaga_r <= flaga;
		flagb_r <= flagb;
		flagc_r <= flagc;
		flagd_r <= flagd;
	end	
end

//定时累加器，硬件上电后即一直开始工作
reg [63:0] timerCnt;
always @(posedge clk, negedge rst_n)begin
	if(!rst_n)begin 
		timerCnt <= 64'd0;
	end 
	else begin
		timerCnt <= timerCnt + 64'd1;
	end	
end

//---------------------------------------------------------------------------//
// 实现功能  : USB slavefifo 收发时序
//---------------------------------------------------------------------------//
always @(posedge clk or negedge rst_n) 
	if(!rst_n) begin
		faddr <=  2'b11;	//EP3OUT		
		slrd <=  1'b1;
		sloe <=  1'b1;
		slwr <= 1'b1;
		pktend <= 1'b1;
		slcs <= 1'b0;
		usb_dir <= 1'b0;
		tx_data <=  32'dz;
		state <= 8'd0;
		num <= 32'd0;

		tPcToFpgaStart <= 64'd0;
		tPcToFpgaEnd <= 64'd0;
		tFpgaToPcStart <= 64'd0;
		tFpgaToPcEnd <= 64'd0;
		catch <= 1'b0;
	end 
	else begin
		case(state)
         //---------------------------------------------------------
         // 系统上电,等待1s硬件ready
			IDLE:
				if(num >= 32'd100_000_000) begin	//delay 1s
					num <= 32'd0;
					state <= PC2FPGA_0;
				end 
				else begin
					num <= num + 1'b1;
				end
         //---------------------------------------------------------
         // 下行数据 PC->FPGA
         PC2FPGA_0: begin 
				catch <= 1'b0;

				if(flagb_r == 1'b1) begin 		// EP3OUT非空有数据, FPGA从USB读出数据
					faddr <= 2'b11; 				// 端口指定EP3
					usb_dir <= 1'b0; 				// 数据端口方向input
					tPcToFpgaStart <= timerCnt;
					state <= PC2FPGA_1; 		// 如果USB有待接收数据,则跳转PC2FPGA的接收流程
				end
				// else 
				// 	state <= FPGA2PC_0;
			end

			PC2FPGA_1: begin
				sloe <= 1'b0; 
				slrd <= 1'b0; 
				state <= PC2FPGA_2;
			end

			PC2FPGA_2:	//需等待
				if(num == 2) begin 
					num <= 32'd0; 
					state <= PC2FPGA_3; 
				end 
				else begin 
					num <= num + 1'b1; 
				end
				
			PC2FPGA_3:
			   if(num == 32'd1024) begin
			   	num <= 32'd0;
			   	sloe <= 1'b1;
			      slrd <= 1'b1;
			      state <= PC2FPGA_4;
			   end 
				else begin
			      num <= num + 1'b1;
			      rx_data[num] <= fdata;
			      if(num >= 32'd1022) 
						slrd <= 1'b1;	//提前2个时钟失能USB读
			      else 
						slrd <= 1'b0;
			   end

         PC2FPGA_4:begin 
            // if((rx_data[0]==16'hA5A5)&&(rx_data[1023]==16'h5A5A)) begin
				// 条件判断，暂时不用，如果使用的话，会增加布线复杂度，算法会使用逻辑资源来实现RAM，导致逻辑占用过多
				// 当然可以用FIFO来桥接OUT和IN，实现LOOP功能，解决逻辑占用的问题，这里为了简化暂不使用FIFO
            // end 
            state <= PC2FPGA_5;
         end

         PC2FPGA_5:	//在读出1024*32bit数据后必须等待至少3个100MHz周期,数据Flag才能被正确读取
         begin
            if(num == 32'd3) begin
               num <= 32'd0;
					tPcToFpgaEnd <= timerCnt;
               state <= FPGA2PC_0;
            end else begin
               num <= num + 1'b1;
            end
         end 

         //---------------------------------------------------------------------------//
         //FPGA->PC
         FPGA2PC_0: 
         if(flaga_r == 1'b1) begin  // EP0IN 非满，FPGA可继续向USB写入数据
            faddr <= 2'b00;			// 端口指定EP0IN 
            usb_dir <= 1'b1;			// 数据端口方向output
				tFpgaToPcStart <= timerCnt;
            state <= FPGA2PC_1;   
         end 
			// else
			// 	state <= PC2FPGA_0;
         
         FPGA2PC_1: 						//此状态总共执行0-1024次，即1025次
         if(num == 32'd1024) begin
            num <= 32'd0;
            slwr <= 1'b1;
            state <= FPGA2PC_2; 
         end 
         else begin
            num <= num + 1'b1;
            slwr <= 1'b0;
				tx_data <= rx_data[num];
         end

         FPGA2PC_2: //在写入1024字数据后必须等待至少3个周期数据Flag才能被正确读取
         begin
            if(num == 32'd4) begin
               num <= 32'd0;
					tFpgaToPcEnd <= timerCnt;
					catch <= 1'b1;
               state <= PC2FPGA_0;
            end 
            else begin
               num <= num + 1'b1;
            end
         end

         default: begin
            faddr <=  2'b11;	//EP3OUT
            slrd <=  1'b1;
            sloe <=  1'b1;
            slwr <= 1'b1;
            pktend <= 1'b1;
            slcs <= 1'b0;
            usb_dir <= 1'b0;		
            tx_data <=  32'dz;
            state <= IDLE;
            num <= 32'd0;
            end
      endcase
	end


reg [63:0] tCurPcToFpgaStart;
reg [63:0] tCurPcToFpgaEnd;
reg [63:0] tCurFpgaToPcStart;
reg [63:0] tCurFpgaToPcEnd;
reg [63:0] tLastPcToFpgaStart;
reg [63:0] tLastPcToFpgaEnd;
reg [63:0] tLastFpgaToPcStart;
reg [63:0] tLastFpgaToPcEnd;
reg [2:0] tstate;
 
always @(posedge clk, negedge rst_n)begin
	if(!rst_n)begin 
		tCurPcToFpgaStart <= 64'd0;
		tCurPcToFpgaEnd <= 64'd0;
		tCurFpgaToPcStart <= 64'd0;
		tCurFpgaToPcEnd <= 64'd0;
		tLastPcToFpgaStart <= 64'd0;
		tLastPcToFpgaEnd <= 64'd0;
		tLastFpgaToPcStart <= 64'd0;
		tLastFpgaToPcEnd <= 64'd0;
		tFpgaRead <= 64'd0;
		tFpgaWaitWrite <= 64'd0;
		tFpgaWrite <= 64'd0;
		tPcLoop <= 64'd0;
		tMaxPcLoop <= 64'd0;
		tstate <= 3'd0;
	end 
	else begin
		case(tstate)
		0: begin
			//在catch时刻暂存所有时间数据
			if (catch==1'b1) begin
				tCurPcToFpgaStart <= tPcToFpgaStart;
				tCurPcToFpgaEnd <= tPcToFpgaEnd;
				tCurFpgaToPcStart <= tFpgaToPcStart;
				tCurFpgaToPcEnd <= tFpgaToPcEnd;
				tstate <= tstate + 1'b1;
			end
		end

			//计算时间长度
		1: begin
			tFpgaRead <= tCurPcToFpgaEnd - tCurPcToFpgaStart;
			tFpgaWaitWrite <= tCurFpgaToPcStart - tCurPcToFpgaEnd;
			tFpgaWrite <= tCurFpgaToPcEnd - tCurFpgaToPcStart;
			tPcLoop <= tCurPcToFpgaStart - tLastFpgaToPcEnd;
			tstate <= tstate + 1'b1;
		end

			//暂存为前次时间数据，留作后期使用
		2: begin
			if (tMaxPcLoop > 64'd100_000_000) begin			//如果得到的值大于1S即为无效值
				tMaxPcLoop <= 64'd0;									//原因是定时器开机即启动且中间无停止
			end 
			else if (tMaxPcLoop < tPcLoop) begin
				tMaxPcLoop <= tPcLoop;
			end

			tLastPcToFpgaStart <= tCurPcToFpgaStart;
			tLastPcToFpgaEnd <= tCurPcToFpgaEnd;
			tLastFpgaToPcStart <= tCurFpgaToPcStart;
			tLastFpgaToPcEnd <= tCurFpgaToPcEnd;
			tstate <= 0;
		end

		default: begin
			tstate <= 0;
		end

		endcase
	end	
end

endmodule

