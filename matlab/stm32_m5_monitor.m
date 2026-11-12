function stm32_m5_monitor(portName)
% STM32 Industrial Controller - Milestone 5 UART telemetry monitor.
% Examples:
%   stm32_m5_monitor("COM4")
%   stm32_m5_monitor("COM6")
% COM port must be the MATLAB end of the paired ports, not Proteus COMPIM end.

if nargin < 1 || strlength(string(portName)) == 0
    error('Specify the MATLAB serial port: stm32_m5_monitor("COM4")');
end

s = serialport(string(portName), 115200, "Timeout", 2);
configureTerminator(s, "LF");
flush(s);

f = figure('Name', 'STM32 Industrial Controller - M5 UART / MATLAB', ...
           'NumberTitle', 'off', 'Color', 'w');
t = tiledlayout(f, 3, 1, 'TileSpacing', 'compact');
title(t, 'UART Telemetry (115200 8N1)');

a1 = nexttile(t);
p1 = animatedline(a1, 'LineWidth', 1.5, 'MaximumNumPoints', 500);
ylabel(a1, 'ADC (mV)'); ylim(a1, [0 3300]); grid(a1, 'on');

a2 = nexttile(t);
p2 = animatedline(a2, 'LineWidth', 1.5, 'MaximumNumPoints', 500);
ylabel(a2, 'PWM (%)'); ylim(a2, [-5 105]); grid(a2, 'on');

a3 = nexttile(t);
p3 = animatedline(a3, 'LineWidth', 1.5, 'MaximumNumPoints', 500);
ylabel(a3, 'RUNNING (0/1)'); xlabel(a3, 'STM32 time (s)');
ylim(a3, [-0.1 1.1]); yticks(a3, [0 1]); grid(a3, 'on');

windowSeconds = 60;
validSamples = 0;
while isgraphics(f)
    try
        raw = strtrim(char(readline(s)));
    catch ME
        if ~isgraphics(f)
            break;
        end
        warning('Serial read failed: %s', ME.message);
        break;
    end
    if isempty(raw) || ~startsWith(string(raw), "M5,")
        continue;
    end
    fields = split(string(raw), ",");
    if numel(fields) ~= 6
        continue;
    end
    numbers = str2double(fields(2:6));
    if any(~isfinite(numbers))
        continue;
    end

    seconds = numbers(1) / 1000;
    adc_mV = numbers(3);
    pwm_percent = numbers(4);
    running = numbers(5);

    addpoints(p1, seconds, adc_mV);
    addpoints(p2, seconds, pwm_percent);
    addpoints(p3, seconds, running);
    validSamples = validSamples + 1;

    xmin = max(0, seconds - windowSeconds);
    xmax = max(windowSeconds, seconds);
    xlim(a1, [xmin xmax]);
    xlim(a2, [xmin xmax]);
    xlim(a3, [xmin xmax]);
    title(t, sprintf('STM32 M5 | ADC %u mV | PWM %u%% | %s | Samples %u', ...
           round(adc_mV), round(pwm_percent), ...
           string(ternaryState(running)), validSamples));
    drawnow limitrate;
end
clear s; % closes the serial port when the window is closed
end

function label = ternaryState(value)
if value >= 0.5
    label = "RUNNING";
else
    label = "IDLE";
end
end
