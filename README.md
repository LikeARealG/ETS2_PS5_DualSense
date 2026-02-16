# **ETS2 PS5 DualSense** - DOWNLOAD FROM [**RELEASES**](https://github.com/LikeARealG/ETS2_PS5_DualSense/releases/tag/v1.0) 

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

## **Installation**

   * Download the archive from [**RELEASES**](https://github.com/LikeARealG/ETS2_PS5_DualSense/releases/tag/v1.0) 
   * Navigate to your Euro Truck Simulator 2 installation directory.  
   * Place the scs-telemetry.dll file inside the following folder: steamapps\\common\\Euro Truck Simulator 2\\bin\\win\_x64\\plugins.
   
     **Important:** Ensure the "ds5w_x64.dll" file is in the same folder as the “ETS2_PS5_DualSense.exe” file.
      
     **Note:** If the “plugins” folder does not exist inside win\_x64, you must create it.    
   

## **How to use**

1. Run the “ETS2\_PS5\_DualSense.exe” application. Does not matter whether you run it before or after you start the game.
2. A console window will appear, displaying live telemetry and effect status. You can minimize this window while you play.  
3. To exit the application, simply press the **PlayStation button** on your controller or just close the window.

## **For _DualSense Edge_ users**

This application is made for the standard version of the controller, not for the Edge one, HOWEVER, you can use another software that emulates a standerd DualSense controller. For example [**DualSenseX**](https://store.steampowered.com/app/1812620/DSX/) with it's [**DLC**](https://store.steampowered.com/app/2345650/DSX__Virtual_DualSense_Emulation/). <sub>I wont provide a link but there is a ~~cracked~~ version of this software on the internet.....</sub>-

## **Updates**

17.02.2026 - Added config file setup(will auto-create next to .exe)

## **Contact**

If you wish to contact me for info/suggestions or anything else you can contact me on [**discord**](https://discordapp.com/users/383656009497051137) (LikeARealG)
