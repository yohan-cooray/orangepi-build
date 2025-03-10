BOOTSCRIPT="boot-meson64.cmd:boot.cmd"
BOOTENV_FILE='meson.txt'
LINUXFAMILY=meson64
ARCH=arm64
#SERIALCON=ttyAML0
SERIALCON=ttyS0
SRC_LOADADDR='LOADADDR=0x1080000'
OVERLAY_PREFIX='meson'

# this family does not need it
ATF_COMPILE="no"

# Set CPUMIN et al if not already set in the board config.
CPUMIN=${CPUMIN:-500000}
CPUMAX=${CPUMAX:-1536000}
GOVERNOR=${GOVERNOR:-ondemand}

write_uboot_platform() {
	dd if=$1/u-boot.bin of=$2 bs=1 count=442 conv=fsync >/dev/null 2>&1
	dd if=$1/u-boot.bin of=$2 bs=512 skip=1 seek=1 conv=fsync >/dev/null 2>&1
}

family_tweaks_bsp() {
	mkdir -p $destination/etc/udev/rules.d
	mkdir -p $destination/usr/local/bin

	mkdir -p "$destination"/etc/X11/xorg.conf.d
	case "${BOARD}" in
		"orangepi3plus")
			cat <<- EOF > "$destination"/etc/X11/xorg.conf
				Section "Device"
				    Identifier  "DRM Graphics Acclerated"

				    ## Use modesetting and glamor
				        Driver      "modesetting"
				        Option      "AccelMethod"    "glamor"     ### "glamor" to enable 3D acceleration, "none" to disable.
				        Option      "DRI"            "2"
				        Option      "Dri2Vsync"      "true"
				        Option      "TripleBuffer"   "True"
				    ## End glamor configuration

				    EndSection

				    Section "Screen"
				        Identifier "Default Screen"
				            SubSection "Display"
				                Depth 24
				            EndSubSection
				        EndSection
			EOF
			;;
	esac
}

# this helper function includes postprocess for u200 and its variants.
# $1 PATH for uboot blob repo
# $2 family g12a or g12b
uboot_g12_postprocess() {
	mv -v u-boot.bin bl33.bin

	$1/blx_fix.sh $1/bl30.bin \
	        zero_tmp \
	        bl30_zero.bin \
	        $1/bl301.bin \
	        bl301_zero.bin \
	        bl30_new.bin bl30

	$1/blx_fix.sh $1/bl2.bin \
	        zero_tmp \
	        bl2_zero.bin \
	        $1/acs.bin \
	        bl21_zero.bin \
	        bl2_new.bin bl2

	$1/aml_encrypt_$2 --bl30sig \
	        --input bl30_new.bin \
	        --output bl30_new.bin.g12.enc \
	        --level v3
	$1/aml_encrypt_$2 --bl3sig \
	        --input bl30_new.bin.g12.enc \
	        --output bl30_new.bin.enc \
	        --level v3 --type bl30
	$1/aml_encrypt_$2 --bl3sig \
	        --input $1/bl31.img \
	        --output bl31.img.enc \
	        --level v3 --type bl31
	$1/aml_encrypt_$2 --bl3sig \
	        --input bl33.bin \
	        --compress lz4 \
	        --output bl33.bin.enc \
	        --level v3 --type bl33
	$1/aml_encrypt_$2 --bl2sig \
	        --input bl2_new.bin \
	        --output bl2.n.bin.sig
	if [ -e $1/lpddr3_1d.fw ]; then
	        $1/aml_encrypt_$2 --bootmk --output u-boot.bin \
	                --bl2 bl2.n.bin.sig \
	                --bl30 bl30_new.bin.enc \
	                --bl31 bl31.img.enc \
	                --bl33 bl33.bin.enc \
	                --ddrfw1 $1/ddr4_1d.fw \
	                --ddrfw2 $1/ddr4_2d.fw \
	                --ddrfw3 $1/ddr3_1d.fw \
	                --ddrfw4 $1/piei.fw \
	                --ddrfw5 $1/lpddr4_1d.fw \
	                --ddrfw6 $1/lpddr4_2d.fw \
	                --ddrfw7 $1/diag_lpddr4.fw \
	                --ddrfw8 $1/aml_ddr.fw \
	                --ddrfw9 $1/lpddr3_1d.fw \
	                --level v3
	else
	        $1/aml_encrypt_$2 --bootmk --output u-boot.bin \
	                --bl2 bl2.n.bin.sig \
	                --bl30 bl30_new.bin.enc \
	                --bl31 bl31.img.enc \
	                --bl33 bl33.bin.enc \
	                --ddrfw1 $1/ddr4_1d.fw \
	                --ddrfw2 $1/ddr4_2d.fw \
	                --ddrfw3 $1/ddr3_1d.fw \
	                --ddrfw4 $1/piei.fw \
	                --ddrfw5 $1/lpddr4_1d.fw \
	                --ddrfw6 $1/lpddr4_2d.fw \
	                --ddrfw7 $1/diag_lpddr4.fw \
	                --ddrfw8 $1/aml_ddr.fw \
	                --level v3
	fi
}
