def printhead (title, full=True):
    hashedline = "################################################################"
    if full is False:
        print(f'\n***\t{title:s}\t***')
    else:
        print(f'\n{hashedline:s}\n***\t{title:s}\t***\n{hashedline:s}')

def linenr (nr):
    print(f'\n[{nr:d}]:')

