APP=Y
TOPDIR := ../../
include $(TOPDIR)build/core/config.mak

SDK_DIR := ../../sdk
CFLAGS += -I$(SDK_DIR)/include
LDFLAGS += -L$(SDK_DIR)/lib
#LDFLAGS += -lcrypto

# For debug model, verbose log message
#CFLAGS += -DDEBUG

#GOAL = $(APPLICATION_OUT_DIR)/system/bin/mtd-rw-tool
GOAL = mtd-rw-tool

CPP_SOURCE = mtd-rw-tool.c

OBJS += mtd-rw-tool.o

LIBS =

.PHONY: all
all:$(GOAL)
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(PROJECT_DIR)/system/bin
	@$(CP) -u $(GOAL) $(PROJECT_DIR)/system/bin/$(GOAL)

$(GOAL):$(OBJS)
	$(MSG_SPLIT_LINE)
	$(MSG_LINKING)
	$(TARGET_CC) $(CPP_SOURCE) -o $(GOAL) $(CFLAGS) $(LDFLAGS)
	$(MSG_GOAL_OK)

.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -rf $(GOAL) $(OBJS) $(OBJS:.o=.d)

.PHONY: release
release:

-include $(OBJS:.o=.d)