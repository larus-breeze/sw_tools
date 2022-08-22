plot(time, x(93,:))
grid
hold
plot(time, x(94,:))
plot(time, x(95,:))
plot(time, sqrt( x(93,:).^2+x(94,:).^2+x(95,:).^2))
legend('North','East','Down','Magnitude');
xlabel('Time / min.');
ylabel('Mag. Induction norm.');
title('Magnetic Induction World Frame Magnetic AHRS');

a=100*6000;
e=350*6000;

std( sqrt( x(93,a:e).^2+x(94,a:e).^2+x(95,a:e).^2))*180/pi
