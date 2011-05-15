######################################################################
#
#   Copyright (C) 2005 Jan Becvar
#   Copyright (C) 2005 soLNet.cz
#
######################################################################

require File.join(File.dirname(__FILE__), 'utils')

class TestString < Test::Unit::TestCase
    S = "ŠkOlA ŽiVotA. ".recode
    S_BAD = S.to_wrong

    def test_empty_string
        assert_equal(0, ''.u_size)
        assert_equal('', ''.u_upcase)
        assert_equal('', ''.u_upcase!)
    end

    def test_u_size
        #size = $U_ENCODING == :utf8 ? 14 : 15
        size = 14

        assert_equal(size, S.u_size)
        assert_equal(size, S_BAD.u_size)

        assert_equal(size, S.u_length)
        assert_equal(size, S_BAD.u_length)
    end

    def do_method_test(method, result, multiple = 1, *args)
        args << :cs

        s = S * multiple
        assert_equal(result * multiple, s.send(method, *args))
        assert_equal(S * multiple, s)

        s_bad = S_BAD * multiple
        if $U_ENCODING == :utf16
            assert_nothing_raised(Exception) { s_bad.send(method, *args) }
        else
            assert_raise(UInvalidCharError) { s_bad.send(method, *args) }
        end
        assert_equal(S_BAD * multiple, s_bad)
    end

    def do_method_test!(method, result, multiple = 1, *args)
        args << :cs
        assert_equal(result * multiple, (S * multiple).send(method, *args))

        if $U_ENCODING == :utf16
            assert_nothing_raised(Exception) { (S_BAD * multiple).send(method, *args) }
        else
            s_bad = S_BAD * multiple
            assert_raise(UInvalidCharError) { s_bad.send(method, *args) }
            assert_equal(S_BAD * multiple, s_bad)
        end
    end

    def test_u_upcase
        s = "ŠKOLA ŽIVOTA. ".recode
        (1..1000).step(100) do |i|
            do_method_test(:u_upcase, s, i)
            do_method_test!(:u_upcase!, s, i)
        end
    end

    def test_u_downcase
        s = "škola života. ".recode
        (1..1000).step(100) do |i|
            do_method_test(:u_downcase, s, i)
            do_method_test!(:u_downcase!, s, i)
        end
    end

    def test_u_capitalize
        s = "Škola života. ".recode
        do_method_test(:u_capitalize, s)
        do_method_test!(:u_capitalize!, s)
    end

    def test_u_titlecase
        s = "Škola Života. ".recode
        (1..1000).step(100) do |i|
            do_method_test(:u_titlecase, s, i, :words)
            do_method_test!(:u_titlecase!, s, i, :words)
        end

        s = "Škola života. ".recode
        (1..1000).step(100) do |i|
            do_method_test(:u_titlecase, s, i, :sentences)
            do_method_test!(:u_titlecase!, s, i, :sentences)
        end

        s = "Škola života. ".recode
        do_method_test(:u_titlecase, s, 1, :string)
        do_method_test!(:u_titlecase!, s, 1, :string)

        assert_raise(ArgumentError) { S.u_titlecase(:lines) }
    end

    def test_u_split_to
        s = ["Š", "k", "O", "l", "A", " ", "Ž", "i", "V", "o", "t", "A", ".", " "].map {|t| t.recode}
        assert_equal(s, S.u_split_to(:chars, :cs))
        assert_equal(s, S.u_split_to(:chars32, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_split_to(:chars, :cs) }
        assert_nothing_raised(Exception) { S_BAD.u_split_to(:chars32) }

        s = [352, 107, 79, 108, 65, 32, 381, 105, 86, 111, 116, 65, 46, 32]
        assert_equal(s, S.u_split_to(:code_points, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_split_to(:code_points) }

        s = ["ŠkOlA", " ", "ŽiVotA", ".", " "].map {|t| t.recode}
        assert_equal(s, S.u_split_to(:words, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_split_to(:words, :cs) }

        add = 'Čěš -^*%*( 3'.recode
        ss = S + add
        s = [S, add]
        assert_equal(s, ss.u_split_to(:sentences, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_split_to(:sentences, :cs) }

        s = ["ŠkOlA ", "ŽiVotA. "].map {|t| t.recode}
        assert_equal(s, S.u_split_to(:lines, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_split_to(:lines, :cs) }

        assert_raise(ArgumentError) { S.u_split_to(:huh) }
    end

    def test_u_count
        assert_equal(14, S.u_count(:chars, :cs))
        assert_equal(14, S.u_count(:chars32, :cs))
        assert_equal(14, S.u_count(:code_points, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_count(:chars, :cs) }
        assert_nothing_raised(Exception) { S_BAD.u_count(:chars32) }
        assert_nothing_raised(Exception) { S_BAD.u_count(:code_points) }

        assert_equal(5, S.u_count(:words, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_count(:words, :cs) }

        add = 'Čěš -^*%*( 3'.recode
        ss = S + add
        assert_equal(2, ss.u_count(:sentences, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_count(:sentences, :cs) }

        assert_equal(2, S.u_count(:lines, :cs))
        assert_nothing_raised(Exception) { S_BAD.u_count(:lines, :cs) }

        assert_raise(ArgumentError) { S.u_count(:huh) }
    end
end

