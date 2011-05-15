######################################################################
#
#   Copyright (C) 2005 Jan Becvar
#   Copyright (C) 2005 soLNet.cz
#
######################################################################

require File.join(File.dirname(__FILE__), 'utils')

Dir.foreach(File.dirname(__FILE__)) do |f|
    next unless f =~ /^test_.*\.rb$/

    require(File.join(File.dirname(__FILE__), f))
end

