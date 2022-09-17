clear;
f=fopen(['/home/schaefer.open/Flight_Logs/220725_OM_Data/20220724_STGT.f50.f112'],'r');
x=fread(f,[112,inf],'float32');
fclose(f);

time=linspace(0,length(x)/100/60,length(x));
