#ifndef FIRMWARE_H
#define FIRMWARE_H

usbd_device *mod_cdcacm_new(void);
void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep);

#endif
