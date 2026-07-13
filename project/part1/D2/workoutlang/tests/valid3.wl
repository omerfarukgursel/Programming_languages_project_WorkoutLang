// Test Program 3: All types, nested if, bool expressions
func isHeavy(s: int, r: int) : bool {
    return s * r > 30
}

workout FullBody {
    day Monday {
        set deadlift reps 3 x 5 rest 3min
        set benchpress reps 4 x 8 rest 2min
    }
    day Thursday {
        set squat reps 4 x 6 rest 2min
        set overhead_press reps 3 x 10 rest 90s
    }
}

routine periodization {
    repeat 8 weeks {
        load FullBody
        bool heavy = isHeavy(4, 8)
        if heavy && true {
            print "Deload recommended"
        }
        if !heavy {
            print "Add weight next session"
        }
    }
}
