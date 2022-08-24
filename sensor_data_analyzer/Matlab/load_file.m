clear;
f=fopen(['../20220724_OM_STGT.f50.f111'],'r');
%f=fopen(['../20220727_Felix.f50.f111'],'r');

x=fread(f,[111,inf],'float32');
fclose(f);

time=linspace(0,length(x)/100/60,length(x));
