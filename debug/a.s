section .text
  global _start

_start:
	mov ecx, [ebp-8]
	mov edx, [ebp-4]
	mov eax, [edx + ecx]
	mov [ecx + edx*4], eax
	mov eax, [edx + ecx * 4]
	mov [ebp -4], eax
	imul eax
	idiv eax
	shl eax, 2
	shr eax, 2
  mov [ecx+edx], eax
  mov [esp], eax
  mov [esp+ 4], eax
  movzx eax, byte [edx + ecx]
  dec eax
  inc eax