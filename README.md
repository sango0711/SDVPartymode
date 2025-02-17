# SDVPartymode
Just a little fun project for a good-bye present using ESP-32


In 2025 the ETAS SDV.SB was released to the internal testing team. The new supercar is enabled by the high power HPC.SB in the center of its E/E Architecture.

One of the most requested and awaited feature is the CycurPARTY™. The feature can be subscribed through a reoccurring fee.

# How to access the CycurPARTY™

After starting the car through the on/off switch, the head- and backlights will turn on and a WiFi AP will be available. If the lights do not come on, refuel the car by the universal fast charger via the mini USB outlet at the trunk. Connect your Device to the open "ETAS SDV.SB" network and access the website under the IP 192.168.4.1 

There you will find a website where you can enter the 8Byte-Hex-formatted code, which you could order through your Manufacturer APP. The FOTA functionality will be available after the 2026 upgrade* (* Extra Costs will apply)

After entering the code, the party mode can be activated in the web interface in this session. To enable the party mode on again, just enter the code again.

	

# Notes from the Testing Team

After analyzing the TOE we found 3 exploitable non-intrusive weaknesses. After which they stopped the analysis, as it seems to be way to easy to circumvent any restriction of the security system.

# Are you able to find them as well?

The testing team was able to retrieve the source code for this SDV and published it. 

The Sourcecode is not needed solve the challenges yourself, but maybe you want to have a look, just for fun.... afterwards. 

Link to Repository






## Well-known Password
 <details> 
  <summary>Hints: Well-known Password</summary>
   We tried known words which are used as placeholders quite often. As it seems the initial default password is still working even in the released version.
</details>
 <details> 
  <summary>Spoiler: Well-known Password</summary>
 DEADBEEF is often used to initially set values in the code. (0xDEADBEEF will work as well)
</details>


## URL Injection

 <details> 
  <summary>Hints: URL Injection</summary>
  After submitting the correct password, the websites redirects to a website which has the CycurPARTY™ Mode enabled. This site can be accessed without the proper code, as the check is not done on this site again.
</details>

 <details> 
  <summary>Spoiler: URL Injection</summary>
  Just access http://192.168.4.1/access/ok this can the changed after entering a false code, which forwards to http://192.168.4.1/access/no 
</details>

## Externally activate the EOL testing
 <details> 
  <summary>Hints: Externally activate the EOL testing </summary>
  The CycurPARTY™ Mode is tested in EOL testing be externally triggering a testing circuit a surprisingly genius mechanism. This can be triggered through household items WITHOUT damages to the car.

</details>
<details> 
  <summary>Spoiler: Externally activate the EOL testing </summary>
Really? Come on! Try more! Think of it like this: At EOL the car drives over something with externally triggers the CycurPARTY™ Mode for testing. The trigger gets triggered. It is a very basic trigger.... no electronics required. Maybe look at your fridge or nametag.


  (it's magnets)
</details>





This page is part of a good-bye present from Dr. Sebastian Burg for a physical Mini-Car which should be located somewhere in the ETAS Home in Feuerbach, probably on the second floor in the security area.




Thanks for the time and opportunities. Hope we will meet again. Bye! 
