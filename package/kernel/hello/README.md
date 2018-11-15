1. put the package under package/kernel
2. select via make menuconfig
   Kernel modules -> Other modules

3. compile
   make package/hello/{clean,prepare,compile} V=s

4. find it in
   build_dir/target-mips_34kc_uClibc-0.9.33.2/linux-ar71xx_generic/hello-1.0
