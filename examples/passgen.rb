$NULL = 0

# String functions

module String
	def length(d:string)
		for len = 0, d[len] != 0, len++; end
		len
	end

	def copy(d:string, s:string)
		i = 0
		while s[i]
			d[i] = s[i++]
		end
	end

	def cat(d:string, s:string)
		i = 0
		len = length(d)
		j = len
		while s[i]
			d[j++] = s[i++]
		end
	end

	def cmp(a:string, b:string)
		diff = 0
		a_len = length(a)
		for i = 0, i < a_len, i++
			diff = diff + a[i] - b[i]
		end
		diff
	end

	def isdigit(n)
		if '0' <= n and n <= '9'
			return 1
		end
		0
	end

	def isalpha(c)
		if 'A' <= c and c <= 'Z'
			return 1
		end
		if 'a' <= c and c <= 'z'
			return 1
		end
		0
	end

	def ishex(c)
		if 'A' <= c and c <= 'F'
			return 1
		end
		if 'a' <= c and c <= 'f'
			return 1
		end
		isdigit(c)
	end

	def atoi(s:string)
		sum = 0; n = 1
		for l = 0, isdigit(s[l]) == 1, l++; n = n * 10; end
		for i = 0, i < l, i++
			n = n / 10
			sum = sum + n * (s[i] - '0')
		end
		sum
	end
end

# Memory
module Memory
	def set(mem:string, n, byte)
		for i = 0, i < byte, i++
			mem[i] = n
		end
	end

	def copy(m1:string, m2:string, byte)
		for i = 0, i < byte, i++
			m1[i] = m2[i]
		end
	end
end

# Math

module Math
	def pow(a, b)
		c = a; b--
		while b-- > 0
			a = a * c
		end
		a
	end

	def abs(a)
		if a < 0; 0 - a else a end
	end

	def fact(n)
		for ret = n--, n > 0, n--
			ret = ret * n
		end
		ret
	end

	def max(a, b)
		if a < b
			b
		else
			a
		end
	end

	def min(a, b)
		if a < b
			a
		else
			b
		end
	end
end

# Secure

module SecureRandom
	def hex(str:string, len)
		gen = File.open("/dev/urandom", "rb")

		bytes = 128
		data:string = Array(bytes)

		File.read(data, bytes, gen)
		chars = 0
		for i = 0, i < bytes and chars < len, i++
			if String.ishex(data[i])
				str[chars++] = data[i]
			else
				File.read(data, bytes, gen)
				i--
			end
		end
		str[chars] = 0
		File.close(gen)
		str
	end
end
# I/O

module IO
	def input(str:string, len)
		f = File.open("/dev/stdin", "w+")
		File.gets(str, len, f)
		File.close(f)
		str
	end
end

# Main

buf:string = Array(256)

if (fp = File.open("includes.rb.test", "r+")) == NULL
	puts "Error: not found file"
else
	File.read(buf, 256, fp)
	printf "%s\n", buf
	File.close(fp)
end

puts "Test: SecureRandom module"
for i = 0, i < 8, i++
	printf "%s\n",  SecureRandom.hex(buf, 16)
end