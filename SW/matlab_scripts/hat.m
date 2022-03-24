function [outVal] = hat(Z)
    if Z <= 128 
        outVal = Z;
    else
        outVal = 256-Z;
    end
end

