#!/usr/bin/env python

import argparse
import json
import subprocess
import sys
import textwrap
import pyperclip
import notify2

try:
    from urllib.parse import urlencode
except ImportError:
    from urllib import urlencode


def valid_style(str):
    str = str.strip()
    formats = ['A', 'P', 'C', 'M', 'N', 'W']

    if str in formats:
        return str
    else:
        msg = 'not a valid style: "{0}"'.format(str)
        raise argparse.ArgumentTypeError(msg)


def valid_title(str):
    if not str:
        parser.print_help(sys.stderr)
        print('\ntcc: error: too few arguments')
        sys.exit(1)


parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description=textwrap.dedent('''\
    format a title based on 1 of 6 industry standard style guides:

      A: Associated Press Stylebook
      P: Publication Manual of the American Psychological Association
      C: Chicago Manual of Style (default)
      M: MLA Handbook
      N: The New York Times Manual of Style and Usage
      W: Wikipedia Manual of Style
    '''))

parser.add_argument('--style', default='C', type=valid_style,
                    help='choose one: A, P, C (default), M, N or W')
parser.add_argument('--clipboard', help='use clipboard as input and output', action="store_true")
parser.add_argument('--notify', help='notify when done', action="store_true")

opts = {}
if not sys.stdin.isatty():
    opts = {
        'default': sys.stdin.read().split()
    }

parser.add_argument('title', default=opts.get('default', None),
                    metavar='words', type=str, nargs='*',
                    help='1 or more words (the title) you want to convert')

args = parser.parse_args()
if args.clipboard:
    args.title = pyperclip.paste().split()

def trim(title):
    return ' '.join(title).strip()


def squish(title):
    return ' '.join(title.split())


def replace_chars(str):
    str = str.decode('utf-8')

    # Replace unicode characters.
    str = str.replace(u'\u2013', '-')
    str = str.replace(u'\u201c', '"')
    str = str.replace(u'\u201d', '"')
    str = str.replace(u'\u2018', "`")
    return str.replace(u'\u2019', "'")


def style_enabled(style):
    return 'true' if args.style == style else 'false'

def sendmessage(title, message):
    notify2.init('tcc')
    n = notify2.Notification(title,  
                             message,  
                             "/usr/share/pixmaps/tcc.svg"   # Icon name  
                            )  
    n.show() 
    return

title = squish(trim(args.title))

valid_title(title)

data = urlencode({
    'title': title,
    'styleA': style_enabled('A'),
    'styleP': style_enabled('P'),
    'styleC': style_enabled('C'),
    'styleM': style_enabled('M'),
    'styleN': style_enabled('N'),
    'styleW': style_enabled('W'),
    'preserveAllCaps': 'true'
})

cmd = ('curl -s -A "tcc" -H "X-Requested-With: XMLHttpRequest" --data "{0}" '
       'https://titlecaseconverter.com/cgi-bin/tcc.pl'.format(data))

body = subprocess.check_output(cmd, stderr=subprocess.STDOUT, shell=True)
body_json = json.loads(body)[0]

result = ''

for key, value in body_json.items():
    if key == 'title':
        for k in value:
            joint = replace_chars(k['joint'].encode('utf-8'))
            word = replace_chars(k['word'].encode('utf-8'))
            result += '{0}{1}'.format(joint, word)

result += body_json['lastJoint'].encode('utf-8')

if args.clipboard:
    pyperclip.copy(result)
else:
    print(result)
if args.notify:
    sendmessage("tcc", "converted to Titlecase: "+ result)
