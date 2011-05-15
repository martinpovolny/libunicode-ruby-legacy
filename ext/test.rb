#!/usr/bin/ruby -Ku
require './unicode.so'

# def p(*a)
# end
# 
# def puts(*a)
# end

class A < UTime
end

def dodo
  # c = UCollator.new(:cs_CS)
  # p ["c", 'č', 'Č', 'C', 'ch', 'h', 'i', 'd', 'ď', 'a', 'á', 'A', 'Á'].sort {|a, b| c.cmp(a, b)}

  #$U_ENCODING = :UTF16
  #p $U_ENCODING

  #  p c.cmp(UString.new('čšěčě'), UString.new('řčěš'))
  #  p c.locale
  #  p c.clone
  #  p c.dup
  #
  #  p UString.new('#dup').dup
  #
  #  puts
  #
  #  p UTF8.size("xx\xffČŠĚČĚa")
  #  p UTF8.length('čšěčěa')
  #
  #  s = UString.new("xxČŠĚČĚa")
  #  p s.size
  #  p s.length
  #  s = s.downcase(:cs_CZ)
  #  p s
  #  s = s.upcase(:cs_CZ)
  #  p s

  puts 

  d = UTime.new('Europe/Prague', :cs)
  d.clone
  p d
  p d.type
  p d.year
  p d.month
  p d.day
  p d.hour
  p d.hour12
  p d.wday
  p d.zone_offset
  p d.dst_offset
  p d.zone
  d.hour = 12
  p d.hour
  d.zone = 'Europe/Prague'
  p d.zone
  p d.hour
  p d.locale
  p d.to_time
  p d.to_f
  p d.to_i

  puts

  df = UTime::Format.new(:full, :full, 'GMT', :cs)
  p df
  p df.dup
  p df.zone
  p df.locale
  p df.format(d)
  p df.zone = 'Europe/London'
  p df.format(d)

  puts

  d1 = df.parse('úterý, 2. srpna 2005 9:19:05 CEST')
  p d1
  p d1.locale
  p d1 <=> d 
  p d1 > 1000.0
  p d1 > Time.now
  p d1 == d
  p d1.eql?(d)
  h = { d1 => 'xx' }
  p h[d1]

  puts

  a = A.new
  p a.eql?(UTime.at(a.to_f, ''))
  p a == UTime.at(a.to_f)

  puts

  df.zone = nil
  p df.format(UTime.now)
  p df.format(UTime.at(Time.now))

  puts

  p d1 + 10.2
  p d1 - 10.2
  p d1 - UTime.now
  p d1 - Time.now
  p d1.succ

  puts

  d = UTime.now(nil, :cs)
  p d
  d.min -= 60 ## <April 31>.mon + 1 => <May 1>, use:  d.add(:month, 1)
  p d
  p d.lenient
  d.lenient = nil
  p d.lenient
  p d.add!(:mday, 1)
  p d.add(:month, 1)

  p df.parse('úterý, 32. srpna 2005 9:19:05 CEST') rescue true
  
end

dodo

#10.times do 
#    Thread.new do
#        loop do
#            dodo
#        end
#    end
#end
#
#loop do
#    GC.start
#    sleep 0.1
#end

