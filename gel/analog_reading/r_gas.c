#include <stdint.h>
#include "r_gas.h"


#define TABLE_SIZE 34

int32_t temperatures_table[TABLE_SIZE]={
	-70000,	-65000,	-60000,	-55000,	-50000,	-45000,	-40000,	-35000,
	-30000,	-25000,	-20000,	-15000,	-10000,	-5000,	0,	5000,
	10000,	15000,	20000,	25000,	30000,	35000,	40000,	45000,
	50000,	55000,	60000,	65000,	70000,	75000,	80000,	85000,
	90000,	95000
};

int32_t R507_table[TABLE_SIZE] = {
	280,380,510,660,860,1100,1390,1770,2130,
	2600,3150,3750,4500,5310,6240,7220,8460,9780,
	11220,12800,14600,16520,18700,22010,23610,26420,
	29490,32850,36560,40000,50000,60000,70000,80000
};

int32_t R32_table[TABLE_SIZE]  = {
	360,490,650,850,1100,1410,1770,2210,
	2730,3350,4060,4880,5830,6910,8130,9510,
	11070,12810,14750,16900,19280,21900,24780,27950,
	31410,35200,39330,43840,48770,55000,65000,75000,85000,95000
};

int32_t R134a_table [TABLE_SIZE]= {
	0,0,0,0,300,390,510,660,
	840,1060,1330,1640,2010,2430,2930,3500,
	4150,4880,5720,6650,7700,8870,10170,11600,
	13180,14920,16820,18900,
	21170,23640,26330,29260,32440,35910
};

static inline int32_t interpolate_table(int32_t* pressure_table, int32_t pressure_read){
	if(pressure_read<0){
		return temperatures_table[0]; //L' ERRORE DEVE ESSERE RILEVATO NELLA ROUTINE
	}
	if(pressure_read>pressure_table[TABLE_SIZE-1]){
		return temperatures_table[TABLE_SIZE-1]; //L' ERRORE DEVE ESSERE RILEVATO NELLA ROUTINE
	}
	//cercare i per cui pressure_table[i]<pressure<pressure_table[i+1]
	// punti A(x1,y1) B(x2,y2), sul grafico PT(pressione=x, temperatura=y)
	// (x-x1)/(x2-x1) = (y-y1)/(y2-y1)
	// t= (t2-t1)*(Pr-p1)/(p2-p1)+t1
	// temperature = temperature[i] + (temperature[i+1]-temperature[i])*(Pr-pressure_table[i])/(pressure_table[i+1]-pressure_table[i])
	int i=0;
	for (i=0;i<TABLE_SIZE-1;i++){//SCORRERE FINO A TABLE_SIZE-1 GARANTISCE DI CADERE ALLA PEGGIO NELL'ULTIMO SLOT
		if((pressure_table[i]<pressure_read)&&((pressure_read<pressure_table[i+1]))){
			break;
		}
	}

	return temperatures_table[i] + (temperatures_table[i+1]-temperatures_table[i])*(pressure_read-pressure_table[i])/(pressure_table[i+1]-pressure_table[i]);
}



int32_t r_gas_saturated_pressure_to_temperature(r_gas_gas_type_t gas_type, int32_t pressure_read){
	int32_t * pressure_table;
	switch (gas_type){
		case r_gas_gas_type_r32:{
			pressure_table=R32_table;
			break;
		}
		case r_gas_gas_type_r134a:{
			pressure_table=R134a_table;
			break;
		}
		case r_gas_gas_type_r507:{
			pressure_table=R507_table;
			break;
		}
		default:{
			return 0;
		}
	}
	return interpolate_table(pressure_table,pressure_read);
}

