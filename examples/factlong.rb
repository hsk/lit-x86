$LEN = 1000
$C = 0

def bigPrint(a)
	for i = LEN - 1, i >= 0, i--
		printf "%d", a[i]
	end puts ""
end

def bigSet(a, b)
	c = b
	for i = 0, c, i++
		a[i] = c % 1000000
		c = c / 1000000
	end
end

def bigAdd(a, b)
	c = 0 
	for i = 0, i < LEN or c, i++
		c = c + a[i] + b[i]
		a[i] = c % 1000000
		c = c / 1000000
	end
end

def bigMul(a, b)
	c = 0
	for i = 0, i < LEN or c, i++
		c = c + a[i] * b
		a[i] = c % 1000000
		c = c / 1000000
	end
end


def bigFact(a, max)
	bigSet(a, 1)
	for i = 1, i <= max, i++
		bigMul(a, i)
	end
	a
end

A = Array(LEN)
C = Array(LEN)
bigSet(A, 11000)
bigPrint( bigFact(A, 1000) )
