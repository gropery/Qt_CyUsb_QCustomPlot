transcript on
if {[file exists rtl_work]} {
	vdel -lib rtl_work -all
}
vlib rtl_work
vmap work rtl_work

vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/ip_core/FIFO {E:/ds/HSC/prj/hsc_ex9/ip_core/FIFO/loopback_fifo.v}
vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/source_code {E:/ds/HSC/prj/hsc_ex9/source_code/usb_controller.v}
vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/ip_core/PLL {E:/ds/HSC/prj/hsc_ex9/ip_core/PLL/pll_controller.v}
vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/source_code {E:/ds/HSC/prj/hsc_ex9/source_code/sys_ctrl.v}
vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/source_code {E:/ds/HSC/prj/hsc_ex9/source_code/led_controller.v}
vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/source_code {E:/ds/HSC/prj/hsc_ex9/source_code/hsc.v}
vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/db {E:/ds/HSC/prj/hsc_ex9/db/pll_controller_altpll.v}

vlog -vlog01compat -work work +incdir+E:/ds/HSC/prj/hsc_ex9/simulation/modelsim {E:/ds/HSC/prj/hsc_ex9/simulation/modelsim/tb_hsc.v}

vsim -t 1ps -L altera_ver -L lpm_ver -L sgate_ver -L altera_mf_ver -L altera_lnsim_ver -L cycloneive_ver -L rtl_work -L work -voptargs="+acc"  tb_hsc

add wave *
view structure
view signals
run -all
