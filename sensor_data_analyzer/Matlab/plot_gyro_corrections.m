[B,A]=butter(2, 0.0001);

subplot(3,1,1)
plot(time,x(75,:)*180/pi)
grid
hold
plot(time,filtfilt(B,A,x(75,:)*1800/pi))
xlabel('Time / min.');
ylabel('Correction / deg/s');
title( 'Front Gyro Correction Signal')
axis([0 max(time) -10 10]);
legend('Inst.','Mean*10')

subplot(3,1,2)
plot(time,x(76,:)*180/pi)
grid
hold
plot(time,filtfilt(B,A,x(76,:)*1800/pi))
xlabel('Time / min.');
ylabel('Correction / deg/s');
title( 'Right Gyro Correction Signal')
axis([0 max(time) -10 10]);
legend('Inst.','Mean*10')

subplot(3,1,3)
plot(time,x(77,:)*180/pi)
grid
hold
plot(time,filtfilt(B,A,x(77,:)*1800/pi))
xlabel('Time / min.');
ylabel('Correction / deg/s');
title( 'Down Gyro Correction Signal')
axis([0 max(time) -10 10]);
legend('Inst.','Mean*10')
