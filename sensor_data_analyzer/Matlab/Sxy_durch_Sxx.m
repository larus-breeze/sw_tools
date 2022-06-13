function [Frequenzen, COH] = Sxy_durch_Sxx( input, output, Abtastfrequenz, Window_Length)

Abtastzeit = 1 / Abtastfrequenz;
Frequenzen = 0 : 1 / ( Window_Length * Abtastzeit) : Abtastfrequenz / 2;
Frequenzen = Frequenzen( 1 : length( Frequenzen) -1);
points = length(input);

window = hanning(Window_Length);
window = window / mean( window);

wx = input(1:Window_Length) .* window;
wy = output(1:Window_Length) .* window;
SX = fft(wx);
SY = fft(wy);
SXY = conj( SX) .* SY;
SXX = SX .* conj(SX);
SYY = SY .* conj(SY);

windows = 1;

for P = Window_Length/4 : Window_Length/4 : points - Window_Length
    windows = windows + 1;
    wx = detrend( input(P:Window_Length+P-1)) .* window;
    wy = detrend( output(P:Window_Length+P-1)) .* window;
    SX=fft( wx);
    SY=fft( wy);
    SXY = SXY + conj( SX) .* SY;
    SXX = SXX + SX .* conj(SX);
    SYY = SYY + SY .* conj(SY);
end

imp = ifft( SXY ./ SXX);

SXY = SXY(1:Window_Length/2) / windows;
SXX = SXX(1:Window_Length/2) / windows;
SYY = SYY(1:Window_Length/2) / windows;

Gjw = SXY ./ SXX;

%FigNr = get(gcf,'Number'); 
FigNr = 0; 
FigNr =FigNr+1;
figure(FigNr);

pa=subplot(3,1,3);
COH = abs( SXY .* SXY) ./ abs(SXX) ./ abs(SYY);
COH =sqrt( COH);
semilogx( Frequenzen, COH)
%grid;
hold;
title('Cohaerenz')
xlabel('Frequenz / Hz')

phase = 180/pi*angle( Gjw);
phase( phase<0) = phase( phase<0) + 360;
subplot(3,1,2)
pb=semilogx( Frequenzen, phase);
%grid;
hold;
title('Phase')

subplot(3,1,1)
pc=loglog( Frequenzen, abs( Gjw));
%grid;
hold;
%title(titel)
linkaxes([pa pb pc],'x')
