#!/bin/bash

res=$(curl -s https://animechan.vercel.app/api/random)

anime=$(grep -oP '(?<="anime":")[^"]*' <<< $res)
character=$(grep -oP '(?<="character":")[^"]*' <<< $res)
quote=$(grep -oP '(?<="quote":")[^"]*' <<< $res)

author="$character from $anime"

svg="<svg fill='none' viewBox='0 0 850 900' preserveAspectRatio='xMidYMin slice' xmlns='http://www.w3.org/2000/svg'><foreignObject width='100%' height='100%'><div xmlns='http://www.w3.org/1999/xhtml'><style>@charset 'UTF-8';@import url(https://fonts.googleapis.com/css?family=Source+Sans+Pro:300,200italic);body {background: #eee;font-family: 'Source Sans Pro', sans-serif;font-weight: 300;}.quote-card {background: #212121;color: #fff;padding: 20px;padding-left: 50px;box-sizing: border-box;box-shadow: 0 2px 4px rgba(34, 34, 34, 0.12);position: relative;overflow: hidden;min-height: 120px;border-radius: 10px;}.quote-card p {font-size: 22px;line-height: 1.5;margin: 0;max-width: 80%;}.quote-card cite {font-size: 16px;margin-top: 10px;display: block;font-weight: 200;opacity: 0.8;}.quote-card:before {font-family: Georgia, serif;content: '“';position: absolute;top: 10px;left: 10px;font-size: 5em;color: #9FA8DA;font-weight: normal;}.quote-card:after {font-family: Georgia, serif;content: '”';position: absolute;bottom: -110px;line-height: 100px;right: -32px;font-size: 25em;color: #9FA8DA;font-weight: normal;}@media (max-width: 640px) {.quote-card:after {font-size: 22em;right: -25px;}}</style><blockquote class='quote-card'><p>$quote</p><cite>- $author</cite></blockquote></div></foreignObject></svg>"

echo $svg