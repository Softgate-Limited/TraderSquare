# Python 2.x, 3.x の違いを吸収 (print と urllib2)

from __future__ import print_function

import urllib
try:
    import urllib.request as urllib2
except:
    import urllib2

# 一応 User-Agent は Chrome にして、XML を取得

user_agent = 'Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.63 Safari/537.36'
headers = {'User-Agent': user_agent}

request = urllib2.Request('http://www.forexfactory.com/ffcal_week_this.xml', None, headers)
response = urllib2.urlopen(request)
xml_content = response.read()

# XMl 要素からテキストを取り出すヘルパー関数
def get_text(ev, name):
    t = ev.find(name).text
    return t if t else ''

impact_table = {'Holiday': 0, 'Low': 1, 'Medium': 3, 'High': 5}

# XML をパースしながら CSV 形式で標準出力に書き出し

import xml.etree.ElementTree as etree
from datetime import date, time, datetime

root = etree.fromstring(xml_content)

for ev in root.findall('event'):

    title, country, d, t = get_text(ev, 'title'), get_text(ev, 'country'), get_text(ev, 'date'), get_text(ev, 'time')
    impact, forecast, previous = get_text(ev, 'impact'), get_text(ev, 'forecast'), get_text(ev, 'previous')

    dt = datetime.strptime('%s %s' % (t, d), '%I:%M%p %m-%d-%Y')

    # 祝日は時刻部分を 00:00:00 にする (time())
    if impact == 'Holiday':
        dt = datetime.combine(dt.date(), time())

    print('{0:%Y-%m-%d %H:%M:%S},{1},{2},{3},{4},{5},'.format(dt, impact_table[impact], country, title, forecast, previous))
