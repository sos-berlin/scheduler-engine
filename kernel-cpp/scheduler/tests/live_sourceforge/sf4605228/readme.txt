SF Forum: https://sourceforge.net/projects/jobscheduler/forums/forum/486122/topic/4605228

Hi,

it is a little bit tricky to prevent the execution of a job at the last sunday of the month, but it works:
    <run_time>
        <weekdays>
            <day day="1 2 3 4 5 6">
                <period single_start="13:00"/>
            </day>
        </weekdays>
        <monthdays>
            <!-- only at the last sunday of the month, holidays will be respected -->
            <weekday day="7" which="-1">
                <period single_start="13:00"/>
            </weekday>
            <weekday day="7" which="-2">
                <period single_start="13:00" when_holiday="ignore_holiday"/>
            </weekday>
            <weekday day="7" which="-3">
                <period single_start="13:00" when_holiday="ignore_holiday"/>
            </weekday>
            <weekday day="7" which="-4">
                <period single_start="13:00" when_holiday="ignore_holiday"/>
            </weekday>
        </monthdays>
        <holidays>
            <weekdays>
                <day day="7"/>
            </weekdays>
        </holidays>
    </run_time>
    
If you want to exclude a specific monthday, it is a little bit easier:
    <run_time>
        <monthdays>
            <!-- note, that 15 is missing -->
            <day day="1 2 3 4 5 6 7 8 9 10 11 12 13 14 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31">
                <period single_start="13:00"/>
            </day>
        </monthdays>
    </run_time>
    
To prevent the execution of a job at a specific (not predefined) day you can define a holidays file and include 
it into your job(s) with 
        <holidays>
            <include live_file="/holiday.xml"/>
        </holidays>

If it is necessary to define a day for maintenance you have to put it into holiday.xml and it would take effect for
all jobs using this file via live_file="/holiday.xml" (caution: you have to restart the scheduler if you have changed holiday.xml).

The last point seems to be a case for API programming. It is a powerfull possibilty to manage JobScheduler "on the fly". Please read the
document "scheduler-api.pdf" for further information in folder doc of your installation.


    
    
