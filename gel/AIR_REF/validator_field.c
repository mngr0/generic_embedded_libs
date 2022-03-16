#include <atmel_start.h>
#include <stdint.h>
#include "validator_field.h"
#include "myprintf/myprintf.h"

void VF_read_from_flash(validated_field* fields, const uint32_t VF_size, uint32_t address){
	uint32_t page_size;
	uint32_t page_to_read[NVMCTRL_PAGE_SIZE];
	page_size = flash_get_page_size(&FLASH_0);
	
	ASSERT(page_size == NVMCTRL_PAGE_SIZE);
	int cnt_page=0;
	int cnt_VF=0;
	while(cnt_VF < VF_size){
		if(cnt_page==0){
			flash_read(&FLASH_0, address, (uint8_t*)page_to_read, page_size*sizeof(int32_t));
			address+=page_size;
		}
		
		fields[cnt_VF].value = page_to_read[cnt_page];
		//myprintf("param %d = %d\n",cnt_VF,page_to_read[cnt_page]);
		cnt_page++;
		cnt_VF++;
		
		
		if(cnt_page==page_size){
			cnt_page=0;
		}
	}
}

static uint32_t page_to_write[NVMCTRL_PAGE_SIZE];

void VF_write_on_flash(validated_field* fields, const uint32_t VF_size, uint32_t address){
	uint32_t page_size;
	page_size = flash_get_page_size(&FLASH_0);
	ASSERT(page_size == NVMCTRL_PAGE_SIZE);
	int cnt_page=0;
	int cnt_VF=0;
	while(cnt_VF < VF_size){
		page_to_write[cnt_page]=fields[cnt_VF].value;
		//myprintf("param %d = %d\n",cnt_VF,page_to_write[cnt_page]);
		cnt_page++;
		cnt_VF++;
		if((cnt_page==page_size)||(cnt_VF == VF_size-1)){
			flash_write(&FLASH_0, address, (uint8_t*)page_to_write, cnt_page*sizeof(int32_t));
			//flash_append(&FLASH_0, address, (uint8_t*)page_to_write, cnt_page*sizeof(int32_t));
			address+=page_size;
			cnt_page=0;
		}
	}
}



