##############################################################
#
# LDD
#
##############################################################

LDD_VERSION = 439c2e8096117cfd92b9726b91212c0908d4bfc7
LDD_SITE = https://github.com/cu-ecen-aeld/assignment-7-Holmberg18.git
LDD_SITE_METHOD = git


# This tells Buildroot to descend into these directories and build the modules
LDD_MODULE_SUBDIRS = misc-modules scull

define LDD_INSTALL_TARGET_CMDS
    # Install kernel modules (handled by kernel-module infrastructure)
    
    # Install and fix module_load script
    $(INSTALL) -D -m 0755 $(@D)/misc-modules/module_load $(TARGET_DIR)/usr/bin/module_load
    sed -i 's|\./$$(module).ko|/lib/modules/$$(uname -r)/extra/$$(module).ko|g' $(TARGET_DIR)/usr/bin/module_load
    
    # Install module_unload script
    $(INSTALL) -D -m 0755 $(@D)/misc-modules/module_unload $(TARGET_DIR)/usr/bin/module_unload
    
    # Install and fix scull_load script
    $(INSTALL) -D -m 0755 $(@D)/scull/scull_load $(TARGET_DIR)/usr/bin/scull_load
    sed -i 's|\./$$module.ko|/lib/modules/$$(uname -r)/extra/$$module.ko|g' $(TARGET_DIR)/usr/bin/scull_load
    
    # Install scull_unload script  
    $(INSTALL) -D -m 0755 $(@D)/scull/scull_unload $(TARGET_DIR)/usr/bin/scull_unload
endef

$(eval $(kernel-module))
$(eval $(generic-package))