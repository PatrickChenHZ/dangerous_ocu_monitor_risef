/*
Vars that are decleared in global scope

*/

#include "R200.h"

void particle_setup(){
  Serial1.begin(2400);
}

void rfid_setup(){
  Serial2.begin(115200);
}

void barometer_setup(){

}

void dht22_setup(){

}

void gas_setup(){

}


float getVout()
{
	const int startByte=170;
	const int endByte  =255;
	int k=0;
	int data;
	int checkbit;
	int val[7];
	int i;
	while(Serial1.available())
	{
		if(Serial1.available()>7)
		{
			data=Serial1.read();
			if(data==startByte)
			{
				k=0;
				val[k]=data;
			}
			else
			{
				k++;
				val[k]=data;
			}

			if (k==6)
			{for(i=0;i<7;i++)
      {Serial.print(" | ");
      Serial.print(val[i],HEX);
        }
				checkbit=val[1]+val[2]+val[3]+val[4];
				if (checkbit==val[5] && val[6==255])
				{
					float Vout=(val[1]*256.0+val[2])/1024*5.0;
					return Vout;
				}
			}
		}

	}
}


float median_average_filter()
{

	int i,j;
	float filter_temp;
	float filter_sum=0;
	float filter_buf[FILTER_N];

	for (i=0;i<FILTER_N;i++)
	{
		filter_buf[i]=getVout();
    Serial.print("vout: ");
    Serial.print(filter_buf[i],4);
    Serial.println();
	}

	for (j=0;j<FILTER_N-1;j++)
	{
		for (i=0;i<FILTER_N-1-j;i++)
		{
			if (filter_buf[i]>filter_buf[i+1])
			{
				filter_temp=filter_buf[i];
				filter_buf[i]=filter_buf[i+1];
				filter_buf[i+1]=filter_temp;
			}
		}
	}
	for (i=1;i<FILTER_N-1;i++)
	{
		filter_sum+=filter_buf[i];
	}

	return filter_sum/(FILTER_N-2);
}

void particle_read(){
  float aa=median_average_filter();
  microparticle=aa*800;
}
