def fibo n
	if n < 2
		1
	else
		fibo(n - 1) + fibo(n - 2)
	end		
end

n = 40
puts "fibo(" n ") = " fibo n
