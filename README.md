# UML Heterogenous Computing Lab 3
Based on the following resources
https://github.com/ACANETS/dpcpp-tutorial 
https://github.com/ACANETS/eece-6540-labs

# Usage
```shell
ssh devcloud
cloud_login # Arria 10 with OneAPI
tools_setup # Arria 10 with OneAPI

git clone https://github.com/PatrickCPE/heterogenous_compute_lab_3
cd heterogenous_compute_lab_3/build
cmake ..

# If making clean
make clean

# cleaning does not delete the old image file
rm cat-rot.bmp

make
./image-conv.fpga_emu

# Resulting image will be stored in cat-rot.bmp
# I recommend pushing to github to view it, it's simpler
```


