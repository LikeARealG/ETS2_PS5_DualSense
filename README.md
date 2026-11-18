# **ETS2 PS5 DualSense** - DOWNLOAD FROM [**RELEASES**](https://github.com/LikeARealG/ETS2_PS5_DualSense/releases) 

This application provides a rich, detailed haptic feedback profile for Euro Truck Simulator 2, specifically designed for the PlayStation 5 DualSense controller. It reads live telemetry data from the game to create immersive rumble and trigger effects that correspond to the truck's status and driver actions.


This project stands on the shoulders of giants. It would not be possible without the incredible work from:

* **RenCloud** for the [**scs-sdk-plugin**](https://github.com/RenCloud/scs-sdk-plugin), which provides the vital telemetry data from the game.  
* **Ohjurot** for the [**DualSense-Windows**](https://github.com/Ohjurot/DualSense-Windows) library, which allows for detailed control over the DualSense controller's features on PC.

## **Features**

* **Engine Simulation:**  
  * **Engine Cranking:** Feel the rhythmic kick of the starter motor on the right side of the controller when turning the engine on.  
  * **Startup Lurch:** Experience a strong, satisfying rumble on the left side as the engine roars to life.  
  * **RPM Vibration:** The right trigger (accelerator) vibrates to simulate engine strain at very low RPMs (lugging) and near the redline.  

* **Driving & Physics:**  
  * **Gear Jolt:** Get a sharp, two-stage "ka-thunk" jolt through the controller every time you shift gears.  
  * **Hard Braking Rumble:** Feel a strong, pulsating vibration during heavy deceleration, simulating the shudder of the brakes and tires.  
  * **Body Roll Rumble:** As your truck leans into a turn, a pulsating rumble on the same side of the controller signals the fact that you are starting to lean and possibly roll over.  
  * **Brake Trigger Resistance:** The left trigger (brake) resistance increases with your speed, making it feel heavier and more realistic to brake from high speeds.
  * **Acceleration Trigger Resistance:** The left right (acceleration) resistance increases at the start and end of the RPM range, on one end you engine is on the brink of stalling and on the other end you engine is revving on the red line, drive in between these 2 points and there your trigger resistance is at minimum, indicating you've hit the sweet spot.  

* **Events & Alerts:**  
  * **Fines & Collision Alert:** When you receive any type of fine (speeding, red light, collision, wrong way, etc.), the lightbar flashes red and blue, and the rumble motors alternate like police sirens for 5 seconds.  
  * **Refueling:** Feel a "chugging" pulse that is strong when your tank is empty and fades out as it fills to the brim.  

* **Status Indicators:**  
  * **Player LED Fuel Gauge:** The five player indicator LEDs act as a fuel gauge in 20% increments. The lights will "breathe" (blink) when the fuel is in the lower 10% of any bracket.


     | 5 LEDs | Blink | 4 LEDs | Blink | 3 LEDs | Blink | 2 LEDs | Blink | 1 LEDs | Blink  |
     |  :---: | :---: | :---:  | :---: | :---:  | :---: | :---:  | :---: |  :---: |  :---: |
     |100-90% |  90-80%  | 80-70% |  70-60%  | 60-50% |  50-40%  | 40-30% |  30-20%  | 20-10% |   10-0%  |

  * **Status LightBar:** The lightbar wil be diffrent colors depending on the situation.


     | Indicators |   Hazards  | Parking Brake |  Retarder  |          Fines          |
     |   :---:    |    :---:   |     :---:     |    :---:   |          :---:          |
     | 🟢 Green   | 🟡 Yellow |     🔴 Red    | 🔵 Blue   |    🔴 Red and 🔵 Blue  |
    
  * **Mic LED Master Warning:** The microphone LED acts as a master warning indicator:  
    * **Solid On:** A minor warning is active (e.g., low AdBlue, battery issue).  
    * **Pulsing:** A critical warning is active (e.g., low oil pressure, engine overheating, low air pressure, high chassis wear).


### Joystick Rim Steering Mode - OFF by default

Move the joystick around its outer rim to steer, just like turning a real steering wheel.

Here is a demo of how it works plus a quick setup (includes a controller overlay so you see my left stick movement):

[![Watch the Joystick Rim Steering demo on YouTube](https://i.ytimg.com/vi/vnQ1dSEJiCs/hqdefault.jpg)](https://youtu.be/vnQ1dSEJiCs)

The exact steps to enable it are found in the README file that comes in the archive.


## **Installation**

   * Download the archive from [**RELEASES**](https://github.com/LikeARealG/ETS2_PS5_DualSense/releases/tag/v1.0) 
   * Navigate to your Euro Truck Simulator 2 installation directory.  
   * Place the scs-telemetry.dll file inside the following folder: steamapps\\common\\Euro Truck Simulator 2\\bin\\win\_x64\\plugins.
   
     **Important:** Ensure the "ds5w_x64.dll" file is in the same folder as the “ETS2_PS5_DualSense.exe” file.
      
     **Note:** If the “plugins” folder does not exist inside win\_x64, you must create it.    

     **These steps are found in the README file in the archive as well <3**

## **How to use**

1. Run the “ETS2\_PS5\_DualSense.exe” application. Does not matter whether you run it before or after you start the game.
2. A console window will appear, displaying live telemetry and effect status. You can minimize this window while you play.  
3. To exit the application, simply press the **PlayStation button** on your controller or just close the window.

## **For _DualSense Edge_ users**

This application is made for the standard version of the controller, not for the Edge one, HOWEVER, you can use another software that emulates a standerd DualSense controller. For example [**DualSenseX**](https://store.steampowered.com/app/1812620/DSX/) with it's [**DLC**](https://store.steampowered.com/app/2345650/DSX__Virtual_DualSense_Emulation/). <sub>I wont provide a link but there is a ~~cracked~~ version of this software on the internet.....</sub>-

## **Changelog**

17.02.2026 - Added config file setup(will auto-create next to .exe)

17.02.2026 V2 - Expanded config to be able to change the intensity of the vibration + UI changes

18.02.2026 - Fixed issue regarding config file

25.04.2026 - Fixed issue where the exe does not recognize the dll and cfg files

17.11.2026 - Added Joystick Rim Steering mode

## **Contact**

If you wish to contact me for info/suggestions or anything else you can contact me on [**discord**](https://discordapp.com/users/383656009497051137) (LikeARealG)

## **Support the Project** ☕

If you enjoy using this mod and want to support its development, you can buy me a coffee! Any support is greatly appreciated but never required.

[**Support on Ko-fi**](https://ko-fi.com/likearealg)

## ⚖️ License & Redistribution

This project is licensed under the **PolyForm Noncommercial License 1.0**.

* **Personal Use:** You are free to use, modify, and share the code.
* **No Selling:** You may **not** use or redistribute this software for commercial purposes or profit.
* **Attribution:** If you repost or modify this, you **must** credit me (`LikeARealG`) and include the original license.

See the [LICENSE](https://github.com/LikeARealG/ETS2_PS5_DualSense/blob/main/LICENSE) file for the full legal terms.
