function [vario, acc] = kalman_vario( in_alti, in_acc)

T = 0.01;              % sampling time

A=[ 1 T T*T/2 0; 
    0 1 T 0; 
    0 0 1 0;
    0 0 0 1];      % system dynamics

C=[ 1 0 0 0
    0 0 1 1];      % measurement

x = [ in_alti(1) 0 0 -9.81]'; % system start state

size = length(in_alti);

alti  = zeros( 1, size);
vario = zeros( 1, size);
acc   = zeros( 1, size);
acc_offset  = zeros( 1, size);

gain  = zeros( 1, size);

K = [
	   0.022706480781195,   0.000238300640696
	   0.026080120255934,   0.008557096024865
	   0.012200483136450,   0.282217429952530
	  -0.011857330213848,   0.000264240373951
   ];


for i= 1 : size
%    P = A * P * A' + Q;
%    K = P * C' / (C*P*C'+R);
    x = A * x;
    x = x + K * ( [in_alti(i) in_acc(i)]' - C * x);
%    P = ( eye(4) - K * C) * P;
    
    alti(i)  = [1 0 0 0] * x;
    vario(i) = [0 1 0 0] * x;
    acc(i)   = [0 0 1 0] * x;
%    acc_offset(i) = [0 0 0 1] * x;

    gain(i)  = K(2,1);
end
