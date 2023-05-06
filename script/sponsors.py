import requests, json, base64, os, sys

AUTH_TOKEN = os.getenv("AUTH_TOKEN")

def genSponsors() -> str :
    """
    Genera il codice svg per la sezione "Sponsors".
    """

    res = requests.post(
        'https://api.github.com/graphql', 
        headers={
            "Authorization": f"Bearer {AUTH_TOKEN}"
        },
        json={
            "query" : """
                query {
                    user(login:"MainKronos") {
                        sponsorshipsAsMaintainer(activeOnly: false, first: 30)   {
                            nodes {
                                sponsorEntity {
                                    ... on User {
                                        login
                                        avatarUrl
                                    }
                                }
                            } 
                        }
                    }
                }
            """
        }
    )

    base = """
    <svg version="1.1" width="702" height="66" viewBox="0 0 702 66" xmlns="http://www.w3.org/2000/svg">
        <mask id="myMask">
            <circle cx="32" cy="32" r="32" fill="white"/>
        </mask>
        {}
    </svg>
    """
    elem = """
    <g transform="translate({x} {y})">
        <image mask="url(#myMask)" href="data:image/jpeg;base64,{data}" width="64" height="64">
            <title>{name}</title>
        </image>
        <circle cx="32" cy="32" r="32" fill="none" stroke-width="2" stroke="black" />
    </g>
    """

    height = 66

    cord_x = 1
    cord_y = 1
    tmp = ""
    for s in res.json()["data"]["user"]["sponsorshipsAsMaintainer"]["nodes"]:
        sponsor = s["sponsorEntity"]
        with requests.get(sponsor["avatarUrl"]) as r:
            tmp += elem.format(x=cord_x, y=cord_y, name=sponsor["login"], data=base64.b64encode(r.content).decode())
        cord_x += 70
        cord_y += (cord_x // 700) * 66
        cord_x %= 700
    
    return base.format(tmp)
   

def main(argv):
    """
    Aggiorna sponsors.svg.
    """
    
    with open("res/sponsors.svg", 'w') as f:
        f.write(genSponsors())

if __name__ == "__main__":
    main(sys.argv)