#include "OldVMEInterface.hh"

#include "eudaq/Exception.hh"
#include "eudaq/Utils.hh"

extern "C" {
#include "vmedrv.h"
}
#include "vmelib.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using eudaq::to_string;

namespace {
  static void CheckDriver() {
    vmeInfoCfg_t VmeInfo;
    memset(&VmeInfo, 0, sizeof VmeInfo);
    int fd = open("/dev/vme_ctl",O_RDONLY);
    //std::cout << "DEBUG: cme_ctl fd = " << fd << std::endl;
    int status = -1;
    if (fd != -1) status = ioctl(fd, VME_IOCTL_GET_SLOT_VME_INFO, &VmeInfo);
    if (status == -1) {
      if (fd != -1) close(fd);
      EUDAQ_THROW("Unable to talk to VME driver. "
                  "Check that the kernel module is loaded "
                  "and the device files exist "
                  "with the correct properties.");
    }
    close(fd);
  }

}

OldVMEInterface::OldVMEInterface(unsigned long base, unsigned long size,
                                 int awidth, int dwidth,
                                 int proto, int sstrate)
  : VMEInterface(base, size, awidth, dwidth, proto, sstrate),
    m_fd(-1)
{
  CheckDriver();
  m_fd = open("/dev/vme_m0", O_RDWR);
  if (m_fd == -1) {
    EUDAQ_THROW("Unable to open VME device file,"
                "maybe another process is accessing it.");
  }
  SetWindowParameters();
}

OldVMEInterface::~OldVMEInterface() {
  if (m_fd != -1) close(m_fd);
}

void OldVMEInterface::SetWindowParameters() {
  if (m_awidth != A32) EUDAQ_THROW("Only support A32");
  if (m_dwidth != D32) EUDAQ_THROW("Only support D32");
}

void OldVMEInterface::DoRead(unsigned long offset, unsigned char * data, size_t size) {
  if (size != 4) EUDAQ_THROW("Only support D32 single read access (" + to_string(size) + ")");
  vme_A32_D32_User_Data_SCT_read(m_fd, (unsigned long *)data, m_base + offset);
}

void OldVMEInterface::DoWrite(unsigned long offset, const unsigned char * data, size_t size) {
  if (size != 4) EUDAQ_THROW("Only support D32 single write access (" + to_string(size) + ")");
  vme_A32_D32_User_Data_SCT_write(m_fd, *(const unsigned long *)data, m_base + offset);
}

OldDMAInterface::OldDMAInterface(unsigned long base, unsigned long size,
                                 int awidth, int dwidth,
                                 int proto, int sstrate)
  : VMEInterface(base, size, awidth, dwidth, proto, sstrate)
{
  CheckDriver();
}

OldDMAInterface::~OldDMAInterface() {
}

void OldDMAInterface::SetWindowParameters() {
  if (m_awidth != A32) EUDAQ_THROW("Only support A32");
  if (m_dwidth != D32) EUDAQ_THROW("Only support D32");
  if (m_proto != PMBLT) EUDAQ_THROW("Only support MBLT access");
  if (m_sstrate != SSTNONE) EUDAQ_THROW("SST not supported");
}

void OldDMAInterface::DoRead(unsigned long offset, unsigned char * data, size_t size) {
  vme_A32_D32_User_Data_MBLT_read(size, m_base + offset, (unsigned long *)data);
}

void OldDMAInterface::DoWrite(unsigned long, const unsigned char *, size_t) {
  EUDAQ_THROW("MBLT write access not supported");
}