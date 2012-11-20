for %%i in (*.off) do (
md res
cd res
md %%i_folder
cd %%i_folder
copy ..\..\%%i
..\..\..\bin\testVHACD.exe %%i 30 0.01 0 32 32 8 128 0.001 2000
cd ..
cd ..
)