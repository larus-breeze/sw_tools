[B,A]=butter(2, 0.01);

subplot(2,1,1)
plot(time,(x(33,:)-x(66,:))/9.81*180/pi)
grid
hold
plot(time,filtfilt(B,A,(x(33,:)-x(66,:))/9.81*180/pi))
xlabel('Time / min.');
ylabel('Error / deg.');
title( 'North Axis Angle Error')

subplot(2,1,2)
plot(time,(x(34,:)-x(67,:))/9.81*180/pi)
grid
hold
plot(time,filtfilt(B,A,(x(34,:)-x(67,:))/9.81*180/pi))
xlabel('Time / min.');
ylabel('Error / deg.');
title( 'East Axis Angle Error')

north_error=std(filtfilt(B,A,(x(33,:)-x(66,:))/9.81*180/pi))
east_error=std(filtfilt(B,A,(x(34,:)-x(67,:))/9.81*180/pi))

