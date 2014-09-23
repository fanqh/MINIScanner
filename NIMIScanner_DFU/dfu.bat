cd F:\git\MINIScanner\NIMIScanner_DFU
dfukeygenerate -v -r actnova.txt -o my_stack_key -i "My stack key"
dfukeygenerate -v -r actnova.txt -o my_app_key -i "My application key" 
dfukeyinsert -v -o loader_with_key -l loader_unsigned.xdv -ks my_stack_key.public.key
dfukeyinsert -v -o stack_ps_with_key_unsigned -ps stack_ps_unsigned.psr -ka my_app_key.public.key
dfusign -v -o stack_ps_signed -ps stack_ps_with_key_unsigned.psr -ks my_stack_key.private.key
dfusign -v -o stack_signed -s stack_unsigned.xpv -ks my_stack_key.private.key
dfusign -v -o file_sys_signed -h image.fs -ka my_app_key.private.key 
dfubuild -v -pedantic -f combined.dfu -uv 0x0a12 -up 0x0001 -ui "BlueCore firmware version release-1 with application and persistent store" -s stack_signed.xpv -h file_sys_signed.fs -p3 . stack_ps_signed.stack.psr .
 
