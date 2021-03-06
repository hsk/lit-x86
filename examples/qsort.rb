require "Math"

def qsort a:int[] left right
	l = left; r = right
	pv = l
	while 1
		while a[l] < a[pv]; l += 1; end
		while a[pv] < a[r]; r -= 1; end
		if l >= r 
			break	
		end
		t = a[l]; a[l] = a[r]; a[r] = t;
		l += 1; r -= 1
	end
	if left < l - 1
		qsort a left l-1
	end
	if r + 1 < right
		qsort a r+1 right
	end
end

max = 20
a = new max int

for i in 0..max
	a[i] = Math_rand % 100
	printf "%d " a[i]
end; puts ""

qsort a 0 max

for i in 0..max
	printf "%d " a[i]
end; puts ""
