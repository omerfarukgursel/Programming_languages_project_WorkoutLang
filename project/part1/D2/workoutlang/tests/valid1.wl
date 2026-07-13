// Test Program 1: Upper body workout with a routine
workout UpperBody {
    day Monday {
        set benchpress reps 4 x 10 rest 90s
        set overhead_press reps 3 x 8 rest 60s
        set tricep_dip reps 3 x 12 rest 45s
    }
    day Wednesday {
        set pullup reps 4 x 8 rest 90s
        set row reps 3 x 10 rest 60s
    }
}

routine myWeek {
    int totalSets = 5
    load UpperBody
    if totalSets > 3 {
        print "Heavy week"
    } else {
        print "Light week"
    }
    repeat 4 weeks {
        load UpperBody
    }
}
