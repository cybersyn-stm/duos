learn how to use milkv-duos


1.首先先构建一次sdk
2.然后用sudo注册sdk
3.免密登录，对/home/cybersyn/sdk/duo-buildroot-sdk-v2/device/generic/br_overlay/common注册root/.ssh/authorized_keys


中断：
对需要启用中断的gpio设备树写入#interrupt-cells = <2>;
比如
	gpio1: gpio@03021000 {
		portb: gpio-controller@1 {
			interrupt-controller;
			interrupts = <61 IRQ_TYPE_LEVEL_HIGH>;
			interrupt-parent = <&plic0>;
            #interrupt-cells = <2>;
		};
	};
