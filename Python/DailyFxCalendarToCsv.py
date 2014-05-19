# -*- coding: UTF-8 -*-

# Python 2.x, 3.x の違いを吸収 (print と urllib2)
from __future__ import print_function
try:
    import urllib.request as urllib2
except:
    import urllib2

# ただし 2.x では文字列処理でエラーになるので動作しない

import argparse
import datetime
import re
import sys

parser = argparse.ArgumentParser(description='Convert DailyFX Japan calendar to CSV')
parser.add_argument('-v', '--verbose', dest='verbose', default=False, action='store_true', help='keep original event titles')
parser.add_argument('-d', '--duplicate', dest='duplicate', default=False, action='store_true', help='do not trim duplicate events')
parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'), default=sys.stdout)
args = parser.parse_args()


# 日本時間表記の日時が年初から何週目かを計算する関数
# ただし土曜日の朝7時以降と日曜日は翌週の値が欲しいので+1
def get_week_of_year(dt):
    woy = dt.isocalendar()[1]
    if (dt.isoweekday() == 6 and dt.hour >= 7) or dt.isoweekday() == 7:
        woy += 1
    return woy


def compose_calendar_url(dt):
    woy = get_week_of_year(dt)
    return 'http://www.dailyfx.co.jp/ec_in/{0:%Y}-{1:0>2}.html'.format(dt, woy)


def simplify_text(text):
    text = re.sub(r'</?[a-zA-Z]+\s*[^>]*?>', '', text)
    if not args.verbose:
        text = re.sub('<[^>]*?>', '', text)
        text = re.sub('＜[^＞]*?＞', '', text)
        text = re.sub('\([^>]*?\)', '', text)
        text = re.sub('（[^）]*?）', '', text)
    # 元データは全角スペースでインデントされていることがあるので、
    # 連続する空白（全角も）をひとまとめにする
    text = re.sub(r'\s+', ' ', text)
    return text


# 一応 User-Agent は Chrome にして、XML を取得

user_agent = 'Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/28.0.1500.63 Safari/537.36'
headers = {'User-Agent': user_agent}

request = urllib2.Request(compose_calendar_url(datetime.datetime.now()), None, headers)
response = urllib2.urlopen(request)
content = response.read().decode('UTF-8')

# HTML をパースしながら CSV 形式で標準出力に書き出し

# 国旗アイコンのファイル名から国コードへの変換テーブル
country_table = {'01': 'CH', '02': 'ZA', '03': 'CA', '04': 'NZ', '05': 'AU',
                 '06': 'GB', '07': 'EU', '08': 'US', '09': 'JP', '10': 'CN',
                 '11': 'DE', '12': 'KR', '13': 'RU', '14': 'FR', '15': 'IT',
                 '16': 'ES', '24': 'HK'}

# 国旗アイコンのファイル名から通貨コードへの変換テーブル
currency_table = {'01': 'CHF', '02': 'ZAR', '03': 'CAD', '04': 'NZD', '05': 'AUD',
                  '06': 'GBP', '07': 'EUR', '08': 'USD', '09': 'JPY', '10': 'CNY',
                  '11': 'EUR', '12': 'KRW', '13': 'RUB', '14': 'EUR', '15': 'EUR',
                  '16': 'EUR', '24': 'HKD'}

pattern = r'''<td\s+class="event_time">((\d+):(\d+))?</td>\s+<td\s+class="eventname"><img\s+src=".+?/flag_(\d+).png">(.*?)</td>\s+<td\s+class="rank">(<img\s+src=['"].+?/star_(\d+).png['"]>)?</td>\s+<td\s+class="event_last_data"\s+nowrap="nowrap">(.*?)</td>\s+<td\s+class="event_exp_data"\s+nowrap="nowrap">(.*?)</td>\s+<td\s+class="event_result_data"\s+nowrap="nowrap">(.*?)</td>'''
regex = re.compile(pattern)

daily_blocks = re.split('eventdate">', content)[1:]

output = []
queue = []
for daily_block in daily_blocks:
    date_match = re.match('\s+([0-9-]+)\s+', daily_block)
    if date_match != None:
        curr_datetime = datetime.datetime.strptime(date_match.group(1), '%Y-%m-%d')
        matches = regex.finditer(daily_block)
        for e in matches:
            if not e.group(4) in country_table:
                continue
            if not e.group(2):
                impact = '0'
                correct_hour = hour = 0
                minute = 0
            else:
                impact = e.group(7) or '0'
                correct_hour = hour = int(e.group(2) or '00')
                minute = int(e.group(3) or '00')
            event_date = curr_datetime
            title = simplify_text(e.group(5))
            if hour >= 24:
                # 24時以上なら時間と日付を補正
                correct_hour -= 24
                event_date += datetime.timedelta(days=1)
            line = '{0} {1:0>2}:{2:0>2},{3},{4},{5},{6},{7},{8},{9}'.format(
                event_date.date(), correct_hour, minute,
                country_table[e.group(4)],
                currency_table[e.group(4)],
                impact,
                title,
                e.group(9), e.group(8), e.group(9))
            output.append(line)

output.sort()

if not args.duplicate:
    # 重複イベントは重要度が高いものにまとめて出力
    if len(output) > 1:
        i = 0
        while i < len(output) - 1:
            curr = output[i].split(',')
            j = i + 1
            while j < len(output):
                next = output[j].split(',')
                if curr[:3] == next[:3]:
                    j += 1
                else:
                    print(output[j - 1], file=args.outfile)
                    i = j
                    break
            i = j

        print(output[-1], file=args.outfile)
else:
    for output_line in output:
        print(output_line, file=args.outfile)
