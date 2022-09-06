[B,A]=butter(2, 0.75/100);

plot(time,x(54,:),'LineWidth',2.0,'Color','green')
grid
hold
[B,A]=butter(2, 1/100);
plot(time,filter(B,A,x(57,:)),'LineWidth',2.0,'Color','blue')
plot(time,filter(B,A,x(53,:)),'LineWidth',2.0,'Color','red')
legend('TEK-Vario','Speed-Compensation','Kalman-Vario');
xlabel('Time / min');
ylabel('Vario / m/s');