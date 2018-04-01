#include <stdio.h>    
#include <stdlib.h>    
#include <sys/types.h>    
#include <string.h>    

#include "libusb-1.0/libusb.h"    
//#include "usb.h"    


int interface_ref = 0;    
int alt_interface,interface_number;    

int print_configuration(struct libusb_device_handle *hDevice,
			struct libusb_config_descriptor *config) {
  //char *data;    
  //int index;    
  //data = (char *)malloc(512);    
  //memset(data,0,512);    
  //index = config->iConfiguration;    
  //libusb_get_string_descriptor_ascii(hDevice,index,data,512);    
  printf("\nInterface Descriptors: ");    
  printf("\n\tNumber of Interfaces : %d",config->bNumInterfaces);
  printf("\n\tLength : %d",config->bLength);
  printf("\n\tDesc_Type : %d",config->bDescriptorType);
  printf("\n\tConfig_index : %d",config->iConfiguration);
  printf("\n\tTotal length : %lu",config->wTotalLength);
  printf("\n\tConfiguration Value  : %d",config->bConfigurationValue);
  printf("\n\tConfiguration Attributes : %d",config->bmAttributes);
  printf("\n\tMaxPower(mA) : %d\n",config->MaxPower);
  //free(data);
  //data = NULL;    
  return 0;    
}

/*
struct libusb_endpoint_descriptor* active_config(struct libusb_device *dev,struct libusb_device_handle *handle) {
  struct libusb_device_handle *hDevice_req;    
  struct libusb_config_descriptor *config;    
  struct libusb_endpoint_descriptor *endpoint;    
  int altsetting_index,interface_index=0,ret_active;
  int i,ret_print;
  hDevice_req = handle;
  ret_active = libusb_get_active_config_descriptor(dev,&config);    
  ret_print = print_configuration(hDevice_req,config);    
  for(interface_index=0;interface_index<config->bNumInterfaces;interface_index++) {    
    const struct libusb_interface *iface = &config->interface[interface_index];    
    for(altsetting_index=0;altsetting_index<iface->num_altsetting;altsetting_index++)  {    
      const struct libusb_interface_descriptor *altsetting = &iface->altsetting[altsetting_index];    
      int endpoint_index;    
      for(endpoint_index=0;endpoint_index<altsetting->bNumEndpoints;endpoint_index++) {
	const struct libusb_endpoint_desriptor *ep = &altsetting->endpoint[endpoint_index];    
	endpoint = ep;      
	alt_interface = altsetting->bAlternateSetting;    
	interface_number = altsetting->bInterfaceNumber;    
      }
      printf("\nEndPoint Descriptors: ");    
      printf("\n\tSize of EndPoint Descriptor : %d",endpoint->bLength);    
      printf("\n\tType of Descriptor : %d",endpoint->bDescriptorType);    
      printf("\n\tEndpoint Address : 0x0%x",endpoint->bEndpointAddress);    
      printf("\n\tMaximum Packet Size: %x",endpoint->wMaxPacketSize);    
      printf("\n\tAttributes applied to Endpoint: %d",endpoint->bmAttributes);    
      printf("\n\tInterval for Polling for data Tranfer : %d\n",endpoint->bInterval);    
    }
  }    
  libusb_free_config_descriptor(NULL);    
  return endpoint;    
}    
*/

int read_synchronous(struct libusb_device_handle *handle) {
  int e;
  unsigned char *my_string;
  int received = 0;
  const int BUFFER_LENGTH = 512;
  my_string = (char *)malloc(BUFFER_LENGTH);

  e = libusb_bulk_transfer(handle,0x082,my_string,BUFFER_LENGTH,&received,0); // synchronous
  //e = libusb_interrupt_transfer(handle,0x082,my_string,BUFFER_LENGTH,&received,0); // synchronous
  printf("\n%d %d\n",e,received);
  if(e<0) return e;
  unsigned short chn[24];
  //short chn[24];
  unsigned short tst;
  unsigned short chk;
  //for(int i=0;i!=BUFFER_LENGTH;++i)
  //  printf("%02x | ",my_string[i]);
  //printf("\n");
  //while(1) {
  for(int i=0,j=-1; i<BUFFER_LENGTH; ++i) {
    tst = my_string[i]<<8|my_string[i+1];
    //printf("i=%d | j=%d | %04x\n",i,j,tst);
    if(tst==0xfefe) { // found start!
      j = 0;
      chk = 0;
      i++;
      continue;
    }
    if(j<0) continue;
    if(j<24) {
      chk ^= tst;
      chn[j] = tst;
    }
    if(j==24) { // checksum
      if(1) {
	for(int i=0;i!=12;++i) {
	  int chval = chn[i]-32768;
	  printf("%d | ",chval);
	}
	//printf("\n");
	//for(int i=12;i!=24;++i)
	//  printf("%04x | ",chn[i]);
	if(chk==tst)
	  printf("CHECKSUM OK\n");
	else
	  printf("FAILED %04x != %04x\n",tst,chk);
      } else {
	printf("CHN2 %d \n",chn[2]);
      }
    }
    j++;
    i++; //skip next
  }
  //}
  printf("\n");
  return e;
}    

int main(void) {
  struct libusb_endpoint_descriptor *epdesc;
  struct libusb_interface_descriptor *intdesc;
  int e = 0,config2;
  int i = 0,index,j=0;
  char str1[64], str2[64];
  char found = 0;

  int r = libusb_init(NULL); // returns 0 on success or errorcode on failure
  if(r!=0) {
    printf("Failed to initialise libusb\n");
    return 1;
  }
  
  struct libusb_device **devs;
  ssize_t cnt = libusb_get_device_list(NULL, &devs); // returns ndevices and list to them
  if(cnt<1) {
    printf("There are no USB devices on bus\n");
    return 1;
  }
  printf("Device Count: %d\n-------------------------------\n",cnt);

  struct libusb_device *dev;
  struct libusb_device_handle *handle = NULL;
  struct libusb_device_descriptor desc;
  int fVerbose=1;
  int bFound=0;
  while((dev=devs[i++]) != NULL) {
    printf("\n DEV %d ==> ",i);
    e = libusb_get_device_descriptor(dev, &desc); // returns 0 on success
    if(e<0) break;
    printf(" | DESC");
    e = libusb_open(dev,&handle); // returns 0 on success or errorcode on failure
    if(e<0) break;
    printf(" | OPEN");
    e=libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, (unsigned char*) str1, sizeof(str1));    
    if(e<0) break;
    e = libusb_get_string_descriptor_ascii(handle, desc.iProduct, (unsigned char*) str2, sizeof(str2));
    if(e<0) break;
    if(fVerbose==1) {
      printf("\nDevice Descriptors: ");    
      printf("\n\tVendor ID : %x",desc.idVendor);    
      printf("\n\tProduct ID : %x",desc.idProduct);    
      printf("\n\tSerial Number : %x",desc.iSerialNumber);    
      printf("\n\tSize of Device Descriptor : %d",desc.bLength);    
      printf("\n\tType of Descriptor : %d",desc.bDescriptorType);    
      printf("\n\tUSB Specification Release Number : %d",desc.bcdUSB);    
      printf("\n\tDevice Release Number : %d",desc.bcdDevice);    
      printf("\n\tDevice Class : %d",desc.bDeviceClass);    
      printf("\n\tDevice Sub-Class : %d",desc.bDeviceSubClass);    
      printf("\n\tDevice Protocol : %d",desc.bDeviceProtocol);    
      printf("\n\tMax. Packet Size : %d",desc.bMaxPacketSize0);    
      printf("\n\tNo. of Configuraions : %d\n",desc.bNumConfigurations);
      printf("\nManufactured : %s",str1);
      printf("\nProduct : %s",str2);
      printf("\n----------------------------------------");
    }
    if(desc.idVendor == 0x04b4 && desc.idProduct == 0x1003) {
      printf("\nDevice found");
      bFound = 1;
      break;
    }
  }//end of while
  if(bFound!=1) {
    printf("\nDevice NOT found\n");
    libusb_free_device_list(devs,1);    
    libusb_close(handle);    
    return 1;    
  }
  
  // device opened is "dev" with handle "handle"

  //active_config(dev,handle);

  e = libusb_get_configuration(handle,&config2);
  if(e!=0) {
    printf("\n***Error in libusb_get_configuration\n");
    libusb_free_device_list(devs,1);    
    libusb_close(handle);    
    return -1;    
  }
  //printf("\nConfigured value : %d",config2);    
  if(config2 != 1) {
    printf("\nAttempt to configure the device\n");
    libusb_set_configuration(handle, 1);
    if(e<0) {
      printf("Error in libusb_set_configuration\n");
      libusb_free_device_list(devs,1);
      libusb_close(handle);
      return -1;
    } else
      printf("\nDevice is in configured state!");
  }
  libusb_free_device_list(devs, 1);
  if(libusb_kernel_driver_active(handle, 0) == 1) {
    printf("\nKernel Driver Active");    
    if(libusb_detach_kernel_driver(handle, 0) == 0)    
      printf("\nKernel Driver Detached!");    
    else    
      {    
	printf("\nCouldn't detach kernel driver!\n");    
	libusb_free_device_list(devs,1);    
	libusb_close(handle);    
	return -1;    
      }    
  }
  e = libusb_claim_interface(handle, 0);    
  if(e < 0)    
    {    
      printf("\nCannot Claim Interface");    
      libusb_free_device_list(devs,1);    
      libusb_close(handle);    
      return -1;    
    }    
  else    
    printf("\nClaimed Interface\n");   

  read_synchronous(handle);

  //libusb_free_device_list(devs,1);    
  libusb_close(handle);    

  libusb_exit(NULL);
  return 0;
}

