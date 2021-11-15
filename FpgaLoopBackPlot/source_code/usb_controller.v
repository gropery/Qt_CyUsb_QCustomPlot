/////////////////////////////////////////////////////////////////////////////
// USB��bulk��ʽ����,����FPGA->PC�ϴ�1024*32bit����,����PC->FPGA�´�1024*32bit����
// PC->FPGA�´����ݶ˵�EP3OUT,�ǿձ�־flagb,��ʾ��flagb=1ʱ,FPGA�ɴ�USB�ж���1024*32bit����,faddr <= 2'b11;
// FPGA->PC�ϴ����ݶ˵�EP0IN, ������־flaga,��ʾ��flaga=1ʱ,FPGA����USB��д��1024*32bit����,faddr <= 2'b00;
/////////////////////////////////////////////////////////////////////////////
module usb_controller(
	input clk,		//100MHz
	input rst_n,	
	//FX3 Slave FIFO�ӿ�
	input flaga,	//full flag EP0-IN
	input flagb,	//empty flag EP3-OUT
	input flagc,	//
	input flagd,	//
	//output fx3_pclk,		//Slave FIFOͬ��ʱ���ź�
	output reg slcs,				//Slave FIFOƬѡ�źţ��͵�ƽ��Ч
	output reg slwr,				//Slave FIFOдʹ���źţ��͵�ƽ��Ч
	output reg slrd,				//Slave FIFO��ʹ���źţ��͵�ƽ��Ч
	output reg sloe,				//Slave FIFO���ʹ���źţ��͵�ƽ��Ч
	output reg pktend,			//�������ź�
	output reg [1:0] faddr,		//����FIFO��ַ
	inout[31:0] fdata,			//����

	//�����˿ڣ���ʹ��signalTab����
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
// �Ĵ���ͬ���ⲿ�����ź�
reg flaga_r;
reg flagb_r;
reg flagc_r;
reg flagd_r;

reg usb_dir;						// ����USB data�˿�input����output
reg [31:0] tx_data;				// USB ����������
reg [31:0] num;
reg [31:0] rx_data[1023:0];	// USB ���յ�����

reg [63:0] tPcToFpgaStart;		//USB�´��˵������ݣ�FPGA��ȡ���ݿ�ʼʱ
reg [63:0] tPcToFpgaEnd;		//FPGA��ȡPC�·����ݽ���
reg [63:0] tFpgaToPcStart;		//USB�ϴ��˵��д�룬FPGAд�����ݿ�ʼʱ
reg [63:0] tFpgaToPcEnd;		//FPGAд��CP�ϴ����ݽ���
reg catch;							//�������ʱ��εĽڵ�

//continuous assignment======================================================//
assign fdata = usb_dir? tx_data:32'dz;	

//always block===============================================================//
////---------------------------------------------------------------------------//
// ʵ�ֹ���  : ͬ��USB Flag��ʶ
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

//��ʱ�ۼ�����Ӳ���ϵ��һֱ��ʼ����
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
// ʵ�ֹ���  : USB slavefifo �շ�ʱ��
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
         // ϵͳ�ϵ�,�ȴ�1sӲ��ready
			IDLE:
				if(num >= 32'd100_000_000) begin	//delay 1s
					num <= 32'd0;
					state <= PC2FPGA_0;
				end 
				else begin
					num <= num + 1'b1;
				end
         //---------------------------------------------------------
         // �������� PC->FPGA
         PC2FPGA_0: begin 
				catch <= 1'b0;

				if(flagb_r == 1'b1) begin 		// EP3OUT�ǿ�������, FPGA��USB��������
					faddr <= 2'b11; 				// �˿�ָ��EP3
					usb_dir <= 1'b0; 				// ���ݶ˿ڷ���input
					tPcToFpgaStart <= timerCnt;
					state <= PC2FPGA_1; 		// ���USB�д���������,����תPC2FPGA�Ľ�������
				end
				// else 
				// 	state <= FPGA2PC_0;
			end

			PC2FPGA_1: begin
				sloe <= 1'b0; 
				slrd <= 1'b0; 
				state <= PC2FPGA_2;
			end

			PC2FPGA_2:	//��ȴ�
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
						slrd <= 1'b1;	//��ǰ2��ʱ��ʧ��USB��
			      else 
						slrd <= 1'b0;
			   end

         PC2FPGA_4:begin 
            // if((rx_data[0]==16'hA5A5)&&(rx_data[1023]==16'h5A5A)) begin
				// �����жϣ���ʱ���ã����ʹ�õĻ��������Ӳ��߸��Ӷȣ��㷨��ʹ���߼���Դ��ʵ��RAM�������߼�ռ�ù���
				// ��Ȼ������FIFO���Ž�OUT��IN��ʵ��LOOP���ܣ�����߼�ռ�õ����⣬����Ϊ�˼��ݲ�ʹ��FIFO
            // end 
            state <= PC2FPGA_5;
         end

         PC2FPGA_5:	//�ڶ���1024*32bit���ݺ����ȴ�����3��100MHz����,����Flag���ܱ���ȷ��ȡ
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
         if(flaga_r == 1'b1) begin  // EP0IN ������FPGA�ɼ�����USBд������
            faddr <= 2'b00;			// �˿�ָ��EP0IN 
            usb_dir <= 1'b1;			// ���ݶ˿ڷ���output
				tFpgaToPcStart <= timerCnt;
            state <= FPGA2PC_1;   
         end 
			// else
			// 	state <= PC2FPGA_0;
         
         FPGA2PC_1: 						//��״̬�ܹ�ִ��0-1024�Σ���1025��
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

         FPGA2PC_2: //��д��1024�����ݺ����ȴ�����3����������Flag���ܱ���ȷ��ȡ
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
			//��catchʱ���ݴ�����ʱ������
			if (catch==1'b1) begin
				tCurPcToFpgaStart <= tPcToFpgaStart;
				tCurPcToFpgaEnd <= tPcToFpgaEnd;
				tCurFpgaToPcStart <= tFpgaToPcStart;
				tCurFpgaToPcEnd <= tFpgaToPcEnd;
				tstate <= tstate + 1'b1;
			end
		end

			//����ʱ�䳤��
		1: begin
			tFpgaRead <= tCurPcToFpgaEnd - tCurPcToFpgaStart;
			tFpgaWaitWrite <= tCurFpgaToPcStart - tCurPcToFpgaEnd;
			tFpgaWrite <= tCurFpgaToPcEnd - tCurFpgaToPcStart;
			tPcLoop <= tCurPcToFpgaStart - tLastFpgaToPcEnd;
			tstate <= tstate + 1'b1;
		end

			//�ݴ�Ϊǰ��ʱ�����ݣ���������ʹ��
		2: begin
			if (tMaxPcLoop > 64'd100_000_000) begin			//����õ���ֵ����1S��Ϊ��Чֵ
				tMaxPcLoop <= 64'd0;									//ԭ���Ƕ�ʱ���������������м���ֹͣ
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

