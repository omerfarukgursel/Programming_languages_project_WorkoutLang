// Test Program 2: Functions, while loop, expressions
func totalVolume(s: int, r: int) : int {
    return s * r
}

workout LegDay {
    day Friday {
        set squat reps 5 x 5 rest 3min
        set legpress reps 4 x 12 rest 90s
        set lunge reps 3 x 10 rest 60s
    }
}

routine strengthPlan {
    int week = 1
    while week <= 12 {
        load LegDay
        int vol = totalVolume(5, 5)
        if vol > 20 {
            print "High volume"
        }
        week = week + 1
    }
}
