# HALCOGEN Configuration
This file documents the HAL setup required to get UART working

## Create New Project
Create a new project if you are generating drivers for the first time (just open the project if you have created one already)
1. create new project **File -> New Project**
2. select **TMS570LS1227PGE** from **TMS570LS12x** Family
3. give the project a name and click **OK**

## Setup 
There are only a few things you need to do for the UART setup
1. Go to **TMS570LS1227PGE -> Driver Enable** tab
2. Make sure to check both **Enable SCI Driver** and **Enable SCI2 Driver**
3. Go to **PINMUX -> Pin Muxing** tab
4. Make sure to check **SCI** in **Enable / Disable Peripherals**

Once you are done with the setup generate code by pressing **F5**.
