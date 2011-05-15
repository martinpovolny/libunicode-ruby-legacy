######################################################################
#
#   Copyright (C) 2005 Jan Becvar
#   Copyright (C) 2005 soLNet.cz
#
######################################################################

require 'benchmark'
require './unicode'
require 'iconv'

$what = {}

for t in (ARGV[0] || 'all').split(',')
  $what[t.intern] = true
end

$to_u16 = Iconv.new('utf-16', 'utf-8')

class String
    def to_u16
        str = $to_u16.iconv(self)
        str << $to_u16.iconv(nil)
        str
    end
end

A = 'r'
U8 = 'ř'

num = nil

puts 'Description:'
puts 'A   = ascii char  ("r"); running with $U_ENCODING set to :utf8'
puts 'U8  = utf-8 char  ("ř"); running with $U_ENCODING set to :utf8'
puts 'U16 = utf-16 char ("ř"); running with $U_ENCODING set to :utf16'

puts

if $what[:upcase] or $what[:all]
    puts('-' * 79)

    for chars in [50, 4_000, 50_000]
        num = 3_000_000 / chars
        puts "#{num}.times do"

        Benchmark.bm(34) do |x|
            str = A * chars
            x.report("  (A * #{chars}).upcase") { num.times { str.upcase } }

            x.report("  (A * #{chars}).u_upcase(:cs)") { num.times { str.u_upcase(:cs) } }

            str = U8 * chars
            x.report("  (U8 * #{chars}).u_upcase(:cs)") { num.times { str.u_upcase(:cs) } }

            str = str.to_u16
            $U_ENCODING = :utf16
            x.report("  (U16 * #{chars}).u_upcase(:cs)") { num.times { str.u_upcase(:cs) } }
            $U_ENCODING = :utf8
        end

        puts "end"
    end
end

if $what[:capitalize] or $what[:all]
    puts('-' * 79)

    for chars in [50, 4_000, 50_000]
        num = 3_000_000 / chars
        puts "#{num}.times do"

        Benchmark.bm(34) do |x|
            str = A * chars
            x.report("  (A * #{chars}).capitalize") { num.times { str.capitalize } }

            x.report("  (A * #{chars}).u_capitalize(:cs)") { num.times { str.u_capitalize(:cs) } }

            str = U8 * chars
            x.report("  (U8 * #{chars}).u_capitalize(:cs)") { num.times { str.u_capitalize(:cs) } }

            str = str.to_u16
            $U_ENCODING = :utf16
            x.report("  (U16 * #{chars}).u_capitalize(:cs)") { num.times { str.u_capitalize(:cs) } }
            $U_ENCODING = :utf8
        end

        puts "end"
    end
end

if $what[:split] or $what[:all]
    puts('-' * 79)

    for chars in [50, 4_000, 50_000]
        num = 500_000 / chars
        puts "#{num}.times do"

        Benchmark.bm(34) do |x|
            str = A * chars
            x.report("  (A * #{chars}).split('')") { num.times { str.split('') } }

            x.report("  (A * #{chars}).u_split_to(:chars)") { num.times { str.u_split_to(:chars) } }

            str = U8 * chars
            x.report("  (U8 * #{chars}).u_split_to(:chars)") { num.times { str.u_split_to(:chars) } }
            x.report("  (U8 * #{chars}).u_split_to(:chars32)") { num.times { str.u_split_to(:chars32) } }
            x.report("  (U8 * #{chars}).u_split_to(:cpoints)") { num.times { str.u_split_to(:code_points) } }

            str = str.to_u16
            $U_ENCODING = :utf16
            x.report("  (U16 * #{chars}).u_split_to(:chars)") { num.times { str.u_split_to(:chars) } }
            x.report("  (U16 * #{chars}).u_split_to(:chars32)") { num.times { str.u_split_to(:chars32) } }
            x.report("  (U16 * #{chars}).u_split_to(:cpoints)") { num.times { str.u_split_to(:code_points) } }
            $U_ENCODING = :utf8
        end

        puts "end"
    end
end

if $what[:size] or $what[:all]
    puts('-' * 79)

    [50, 4_000, 50_000].each_with_index do |chars, i|
        num = 100_000_000 / ((i + 1) * 2000)
        puts "#{num}.times do"

        Benchmark.bm(34) do |x|
            str = A * chars
            x.report("  (A * #{chars}).size") { num.times { str.size } }

            x.report("  (A * #{chars}).u_size") { num.times { str.u_size } }
            x.report("  (A * #{chars}).u_count(:chars32)") { num.times { str.u_count(:chars32) } }

            str = U8 * chars
            x.report("  (U8 * #{chars}).u_size") { num.times { str.u_size } }
            x.report("  (U8 * #{chars}).u_count(:chars32)") { num.times { str.u_count(:chars32) } }

            str = str.to_u16
            $U_ENCODING = :utf16
            x.report("  (U16 * #{chars}).u_size") { num.times { str.u_size } }
            x.report("  (U16 * #{chars}).u_count(:chars32)") { num.times { str.u_count(:chars32) } }
            $U_ENCODING = :utf8
        end

        puts "end"
    end
end

if $what[:time] or $what[:all]
    ## Date versus UTime
    puts('-' * 79)

    require 'date'
    d = Date.today
    t = UTime.now

    num = 50_000
    puts "#{num}.times do"

    Benchmark.bm(34) do |x|
    x.report("  Date#+")  { num.times { d += 1 } }
    x.report("  UTime#add(:mday, 1)") { num.times { t = t.add(:mday, 1) } }
    x.report("  UTime#add!(:mday, 1)") { num.times { t.add!(:mday, 1) } }
    end

    puts "end"

    num = 5_000
    puts "#{num}.times do"

    Benchmark.bm(34) do |x|
    x.report("  Date#>>")  { num.times { d = d >> 1 } }
    x.report("  UTime#add(:month, 1)") { num.times { t = t.add(:month, 1) } }
    x.report("  UTime#add!(:month, 1)") { num.times { t.add!(:month, 1) } }
    end

    puts "end"

    puts('-' * 79)
end


