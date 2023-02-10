import requests
import sys, os, re, time, math

GITHUB_TOKEN = os.getenv("GITHUB_TOKEN")


def genActivity() -> str :
	"""
	Genera il codice markdown per la sezione "Activity". Comprende gli ultimi 3 repository modificati e gli ultimi 3 commit effettuati.
	"""

	ret = ""
	for repo in requests.get(
		url="https://api.github.com/user/repos",
		headers={
			"Accept": "application/vnd.github+json",
			"X-GitHub-Api-Version": "2022-11-28"
		},
		auth=("MainKronos",GITHUB_TOKEN),
		params={
			"visibility": "all",
			"affiliation": "owner",
			"sort": "pushed",
			"direction": "desc",
			"per_page": 3,
			"page": 1
		}
	).json():
		ret += "### {} [{}]({})\n> {}\n".format(
			"🔒" if repo["private"] else "🔓",
			repo["name"],
			repo["html_url"],
			repo["description"]
		)

		for commit in requests.get(
			url=f"https://api.github.com/repos/MainKronos/{repo['name']}/commits",
			headers={
				"Accept": "application/vnd.github+json",
				"X-GitHub-Api-Version": "2022-11-28"
			},
			auth=("MainKronos",GITHUB_TOKEN),
			params={
				"author": "MainKronos",
				"per_page": 3,
				"page": 1
			}
		).json():
			message = commit["commit"]["message"].split("\n")[0]
			
			ret += "- 📌 [{}]({})\n".format(
				message,
				commit["html_url"]
			)
	return ret
	
def genQuote() -> str:
	"""
	Genera il codice svg per la sezione "Quote".
	"""
	res = requests.get("https://animechan.vercel.app/api/random").json()

	quote = res["quote"]

	author = res["character"] + " from " + res["anime"]

	height = (math.ceil(len(res["quote"]) / 64) - 1) * 33 + 120

	return r"<svg fill='none' viewBox='0 0 850 "+str(height)+r"""' preserveAspectRatio='xMidYMin meet' xmlns='http://www.w3.org/2000/svg'>
	<foreignObject width='100%' height='100%'>
		<div xmlns='http://www.w3.org/1999/xhtml'>
			<style>
				@charset 'UTF-8';
				@import url(https://fonts.googleapis.com/css?family=Source+Sans+Pro:300,200italic);

				body {
					background: #eee;
					font-family: 'Source Sans Pro', sans-serif;
					font-weight: 300;
				}

				.quote-card {
					background: #212121;
					color: #fff;
					padding: 20px;
					padding-left: 50px;
					box-sizing: border-box;
					box-shadow: 0 2px 4px rgba(34, 34, 34, 0.12);
					position: relative;
					overflow: hidden;
					min-height: 120px;
					border-radius: 10px;
				}

				.quote-card p {
					font-size: 22px;
					line-height: 1.5;
					margin: 0;
					max-width: 80%;
					hyphens: auto;
					text-align: justify;
				}

				.quote-card cite {
					font-size: 16px;
					margin-top: 10px;
					display: block;
					font-weight: 200;
					opacity: 0.8;
				}

				.quote-card:before {
					font-family: Georgia, serif;
					content: '“';
					position: absolute;
					top: 10px;
					left: 10px;
					font-size: 5em;
					color: #9FA8DA;
					font-weight: normal;
				}

				.quote-card:after {
					font-family: Georgia, serif;
					content: '”';
					position: absolute;
					bottom: -110px;
					line-height: 100px;
					right: -32px;
					font-size: 25em;
					color: #9FA8DA;
					font-weight: normal;
				}

				@media (max-width: 640px) {
					.quote-card:after {
						font-size: 22em;
						right: -25px;
					}
				}
			</style>
			<blockquote class='quote-card'>
				<p>"""+quote+r"</p><cite>- "+author+r"""</cite>
			</blockquote>
		</div>
	</foreignObject>
</svg>
"""

def main(argv):
	"""
	Genera la lista degli ultimi rtepository modificati.
	"""

	activity_delimiter = re.compile("(?<=<!-- BEGIN ACTIVITY -->\n).*?(?=<!-- END ACTIVITY -->)", re.DOTALL)
	quote_delimiter = re.compile("(?<=<!-- BEGIN QUOTE -->\n).*?(?=<!-- END QUOTE -->)", re.DOTALL)
	
	data = ""
	with open("README.md", 'r') as f:
		data = activity_delimiter.sub(f"\n{genActivity()}\n", f.read())
		data = quote_delimiter.sub(f"\n<img align='center' src='res/quote.svg?{time.time()}' width='100%'>\n\n", data)

	with open("README.md", 'w') as f:
		f.write(data)
	
	with open("res/quote.svg", 'w') as f:
		f.write(genQuote())

if __name__ == "__main__":
	main(sys.argv)