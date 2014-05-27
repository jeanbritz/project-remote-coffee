% output of analogRead(A9)
x =[
563
579
591
607
624
639
659
680
700
718
734
752
765
779
812
412
203
632
799
344
236
707
743
274
316
764
641
220
423
807
540
202
505
815
481
198
550
813
440
199
602
808
381
208
665
780
310
264
739
697
249
366
792
591
206
468
812
507
199
531
812
465
199
574
813
414
201
630
803
349
231
702
748
275
309
760
646
221
416
807
547
202
503
813
483
198
548
813
440
200
598
811
387
209
662
783
312
262
734
704
251
359
788
598
204
463
813
510
197
527
815
465
198
570
814
416
202
627
804
352
228
700
752
280
305
761
652
224
414
806
551
199
501
812
486
199
546
815
444
199
595
809
387
208
658
783
316
258
732
707
254
355
785
600
206
459
812
512
198
527
815
467
199
569
812
420
202
624
807
355
225
696
754
280
303
756
656
227
408
805
553
203

];

% Constants
N = length(x)
n = 0:(N-1);

% Data window
%w = hamming(N);
w = 1
xw = x.*w;
Xw = fft(xw);

% Plot time-domain signal
figure(1);
plot(n, xw);
xlabel('Time(samples)');
ylabel('Amplitude');

% Plot spectrum
figure(2);
stem(linspace(0,1-1/N,N), abs(Xw));
xlabel('Frequency (cycles/sample)');
ylabel('Spectral magnitude');

y = zeros(1,N);
w0 = 2*pi*40;
for i=3:N-1
y(i) = 1*(x(i) - 2*cos(w0)*x(i-1) + x(i-2));
end
figure(2);
plot(y);

% Plot spectrum
figure(3);

%w = hamming(N);
w = 1
yw = y.*w;
Yw = fft(yw);

figure(4);
stem(linspace(0,1-1/N,N), abs(Yw));
xlabel('Frequency (cycles/sample)');
ylabel('Spectral magnitude');

% yf = fft(y);
% figure(2);
% plot(y);
% figure(3);
% stem(n, abs(yf)/196);

%stem(n, abs(xf)/N);