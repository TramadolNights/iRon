## Notes

I haven't attempted C++ before so some of the code may not be the finest, but it works (I think) and maybe
someone else will find some of the new features useful.

Thanks to lespalt (https://github.com/lespalt/iRon), frmjar (https://github.com/frmjar/iRon)
& diegoschneider (https://github.com/diegoschneider/iRon) for their previous work on this project.

## What's new in this version?

- Added an optional clock in standings footer, shows local time by default or iRacing session time (show_clock_session_time);

- Added an optional weather info in standings footer (e.g. "Partly cloudy / Lightly wet");

- Added a pointless little temp change indictor (+/-) to the temp display in the standings footer, updates every 90 seconds;

- Added an optional joker lap column to standing and relative overlays, showing the number of joker laps taken;
    - It is only displayed if is is a race session, the number of joker laps in the race session is > 0 and show_joker_laps config is set to true;
	- During the race it is either an orange or green box, depending on whether the driver has taken all required joker laps or not;
	- During practice and qualifying it is just a grey box;

- Added an optional tire compound column to the standings overlay (dry / wet);

- Added optional config to display DDU speed in MPH while metric unit is set in iRacing and vice versa with speed_unit config;
	- iRacing doesn't have any flexibility in what units to use, and I want speed in MPH (imperial) & temps in celcius (metric);

- Added ABS and TC indicators to the DDU overlay, they blink on change as per the existing brake bias indicator;

- The oil & water temp indicators in the DDU overlay now slowly flash when warning level is reached;

- The DDU now displays tire temps (live when available) by default, can be changed back to tire wear by setting show_tire_wear config to true;

- Fixed the "error" car icon, the corners weren't quite drawn correctly;

- Car icon file names containing a "_" didn't get displayed, now they do;

- Some random little edits to get rid of some compiler warnings etc.

