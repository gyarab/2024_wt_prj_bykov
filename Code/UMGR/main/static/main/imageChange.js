function ich() {
    if(this.isChanged) {
        document.getElementById("changed").style = "";
        document.getElementById("changed").src = "https://placehold.co/600x400/";
        this.isChanged = false;
    }
    else {
        document.getElementById("changed").style = "border: 5px solid red;";
        document.getElementById("changed").src = "https://placehold.co/600x400/000080/FFFFFF";
        this.isChanged = true;
    }
}