for %%i in (*.off) do (
md res
cd res
md %%i_folder
cd %%i_folder
copy ..\..\%%i
..\..\..\bin\testVHACD.exe %%i 10 0.01 0 10 10 5 5 0.01 1000 > log.txt
cd ..
cd ..
)