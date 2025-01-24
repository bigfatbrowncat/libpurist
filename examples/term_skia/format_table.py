def print_format_table():
    """
    prints table of formatted text format options
    """
    for style in range(8):
        for light_flag in (0, 60):
            for fg in range(30 + light_flag, 38 + light_flag):
                s1 = ''
                for bg in range(40 + light_flag, 48 + light_flag):
                    format = ';'.join([str(style), str(fg), str(bg)])
                    s1 += '\x1b[%sm %s \x1b[0m' % (format, format)
                print(s1)
            print('\n')


print_format_table()
