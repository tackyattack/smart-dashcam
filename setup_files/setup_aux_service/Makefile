# Author:
# Copyright:
# License:
# Date Created:	Sept. 27, 2019
# Resources:	http://www.gnu.org/software/make/manual/make.html
#				http://nuclear.mutantstargoat.com/articles/make/#practical-makefile
# Desc:


# This makes spaces the recipe separator instead of the standard tab '\t' character
#   Comment out the line to default back to tabs, or make it equal to any character you wish to use as a recipe separator
.RECIPEPREFIX +=

SYSTEMD_UNIT_DIR=/etc/systemd/system/


#target … : prerequisites …
#        recipe

# Where
#		target -> name of file generated
#		prerequisites ->  a file that is used as input to create the target
#		recipe -> actions taken by make. Tab is required.

.PHONY: pi0_setup clean clean-all

pi0_setup: $(EXE)
    sudo cp -f dashcam_aux.service $(SYSTEMD_UNIT_DIR)
    sudo chmod 0755 $(SYSTEMD_UNIT_DIR)/dashcam_aux.service
    sudo systemctl enable dashcam_aux
    sudo systemctl daemon-reload
    sudo systemctl restart dashcam_aux
    @echo "Setup main start service for aux unit\n\n"


# make clean will remove any generated files that are not needed for executing or compiling code (ie object files)
clean :

# make clean-all will remove all generated/downloaded files. Should essetially revert directories back to original post clone state
clean-all:
    sudo rm -rf $(SYSTEMD_UNIT_DIR)/dashcam_aux.service  $(SERVICES_DIR)/device-id
