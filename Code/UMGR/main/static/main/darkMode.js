let darkModeToggleValue = false;
let hoverOn = false;

function clearClasses(elem) {
	elem.classList.remove("bi-moon");
	elem.classList.remove("bi-moon-fill");
	elem.classList.remove("bi-sun");
	elem.classList.remove("bi-sun-fill");
}

function darkModeToggle() {
	console.log("DMT");
	darkModeToggleValue = !darkModeToggleValue;

	if(darkModeToggleValue) {
		document.documentElement.setAttribute("data-bs-theme", "dark");
	}
	else {
		document.documentElement.setAttribute("data-bs-theme", "light");
	}

	darkModeHover();
	hoverOn = !hoverOn; //toggle back
}

function darkModeHover() {
	hoverOn = !hoverOn;

	let elem = document.getElementById("darkmodeelem");
	clearClasses(elem);

	if(hoverOn) {
		if(darkModeToggleValue) elem.classList.add("bi-moon-fill");
		else elem.classList.add("bi-sun-fill");
	}
	else {
		if(darkModeToggleValue) elem.classList.add("bi-moon");
		else elem.classList.add("bi-sun");
	}
}