.code
extern exportedFunctions:QWORD
d_Direct3DShaderValidatorCreate9 proc
	jmp exportedFunctions[0*8]
d_Direct3DShaderValidatorCreate9 endp
d_PSGPError proc
	jmp exportedFunctions[1*8]
d_PSGPError endp
d_PSGPSampleTexture proc
	jmp exportedFunctions[2*8]
d_PSGPSampleTexture endp
d_D3DPERF_BeginEvent proc
	jmp exportedFunctions[3*8]
d_D3DPERF_BeginEvent endp
d_D3DPERF_EndEvent proc
	jmp exportedFunctions[4*8]
d_D3DPERF_EndEvent endp
d_D3DPERF_GetStatus proc
	jmp exportedFunctions[5*8]
d_D3DPERF_GetStatus endp
d_D3DPERF_QueryRepeatFrame proc
	jmp exportedFunctions[6*8]
d_D3DPERF_QueryRepeatFrame endp
d_D3DPERF_SetMarker proc
	jmp exportedFunctions[7*8]
d_D3DPERF_SetMarker endp
d_D3DPERF_SetOptions proc
	jmp exportedFunctions[8*8]
d_D3DPERF_SetOptions endp
d_D3DPERF_SetRegion proc
	jmp exportedFunctions[9*8]
d_D3DPERF_SetRegion endp
d_DebugSetLevel proc
	jmp exportedFunctions[10*8]
d_DebugSetLevel endp
d_DebugSetMute proc
	jmp exportedFunctions[11*8]
d_DebugSetMute endp
; d_Direct3DCreate9 proc
;	jmp exportedFunctions[12*8]
; d_Direct3DCreate9 endp
; d_Direct3DCreate9Ex proc
;	jmp exportedFunctions[13*8]
; d_Direct3DCreate9Ex endp
end
