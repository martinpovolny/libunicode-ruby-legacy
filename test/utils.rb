######################################################################
#
#   Copyright (C) 2005 Jan Becvar
#   Copyright (C) 2005 soLNet.cz
#
######################################################################

$:.unshift(File.join(File.dirname(__FILE__), "..", "ext"))

require 'test/unit'
require 'unicode'
require 'iconv'

$U_ENCODING = ARGV[0].intern if ARGV[0]

if $U_ENCODING == :utf16
    class String
        def recode
            Iconv.iconv('utf-16', 'utf-8', self).join[2..-1]
        end

        def to_wrong
            s = dup
            s[2] = 0
            s[3] = 0
            s[4] = 140
            s[5] = 140
            s[-2] = 255
            s[-1] = 255
            s
        end
    end
else
    class String
        def recode
            self
        end

        def to_wrong
            s = dup
            s[3] = 0
            s[5] = 140
            s[-1] = 255
            s
        end
    end
end
