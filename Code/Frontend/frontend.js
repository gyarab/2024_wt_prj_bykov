//enter to search
window.addEventListener("keydown", (e) => {
	if(e.code === "Enter") {
		get_results();
	}
});

function clear_results() {
	document.getElementById("results").innerHTML = ""; //just clear everything inside
}

function make_result_element(aName, aDescription, aDomain, aAuthor = undefined) {
	const container = document.getElementById("results");

	const parent = document.createElement("div");
	parent.className = "result";

	const title = document.createElement("h2");
	title.innerHTML = aName + " (at " + aDomain + ")"; 	//add domain
	const description = document.createElement("p");
	description.innerHTML = aDescription;

	parent.appendChild(title);

	//add ability to click box
	parent.addEventListener("click", () => {
		window.location.href = "//"+aDomain; // so we have an absolute address
	});

	//add hidden crawlbot address (so bots can crawl this even though they shouldnt)
	const hidden_a_tag = document.createElement("a");
	hidden_a_tag.className = "crawl_address";
	parent.appendChild(hidden_a_tag);

	if(aAuthor != undefined) {
		//add author if applicable
		const author = document.createElement("p");
		parent.appendChild(author);
	}

	parent.appendChild(description);

	container.appendChild(parent);
}


//address of our webserver, here localhost
const server_address = "http://127.0.0.1:8080";
//address to show on error
const errorurl = "megapolisplayer.github.io/bugreport";

//defined in WebServer/Serialization.h

const WEBSERVER_ENTRY_NEW = "\u0001";
const WEBSERVER_ENTRY_SPLIT = "\u0002";

function get_results() {
	clear_results();

	const query = document.getElementById("queryinput").value;
	document.getElementById("queryinput").value = ""; //empty input field

	document.getElementById("searched").innerHTML = query.length == 0 ? "nothing" : "\""+query+"\""; //write query (if empty write "nothing")

	if(!query) {
		console.log("Query is empty.");
		return;
	}

	console.log("Query: ", query);
	let req = new XMLHttpRequest();
	req.open("POST", new URL(server_address));

	//callbacks
	req.onerror = () => {
		//timeout handler
		console.error("Request error");
		req.abort();
	};
	req.onload = () => {
		if(req.status != 200) {
			//204 if empty query
			req.abort();
			console.error("Invalid query!");
			return;
		}

		console.log("Response:", req.responseText);
		console.log("Length of response: ", req.response.length);

		//process as string - split by entry
		const entries = req.responseText.split(WEBSERVER_ENTRY_NEW);
		entries.pop(); //remove last element (all entries end with WEBSERVER_ENTRY_NEW, will be empty element after last such char)

		if(req.responseText === "NORESULT" || entries.length == 0) {
			//no results found
			console.log("No results found!");
			make_result_element("No results found!", "We could not find what you are looking for. Check your query and try again.", errorurl);
			return;
		}

		console.log("Entries: ", entries);

		for(const entry of entries) {
			const sections = entry.split(WEBSERVER_ENTRY_SPLIT);
			if(sections.length != 4) {
				console.error("Invalid response!", sections);
				return;
			}
			console.log(entry, " split into ", sections);

			//name(2)author(2)address(2)description(1)...
			make_result_element(sections[0], sections[3], sections[2], sections[1]);
		}
	}

	//actual sending
	try { req.send(query); }
	catch(e) {
		req.abort();
		console.error("Request failed: check if your server is enabled. If the problem persists, contact the author of this program.");
		make_result_element("An error occured!", "We are unable to reach the NPWS server. Please check if the server is enabled and try again. If the problem persists, contact the author of this program by clicking this result.", errorurl);
		return;
	}
}
