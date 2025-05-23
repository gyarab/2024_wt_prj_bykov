from django.http import HttpResponse, JsonResponse
from django.shortcuts import render
from main.models import KeywordList, SearchUser, SearchUserAttributes
from django.views.decorators.csrf import csrf_exempt

@csrf_exempt
def logIn(request):
    if request.method == "GET":
        print(request.POST)
        return render (
            request, "main/login.html", {}
        )
    elif request.method == "POST":
        u = request.POST.get("username")
        p = request.POST.get("password")

        # INCOMPLETE - dont want to use plaintext passwords

        users = SearchUser.objects.all().get(username=u)
        if users is not None:
            print(users)
            return HttpResponse(status=202)
        else:
            return HttpResponse(status=403)
    
    return HttpResponse(status=400)

def getIndex(request):
    kl = KeywordList.objects.all().order_by('keyword')

    if request.GET.get("search"):
        kl = kl.filter(keyword__icontains=request.GET.get("search"))
    # dlango filter field lookup expression
    # __ == .

    context = {
        "svatek": "SOUDRUH JENÍČEK A SOUDRUŽKA MAŘENKA V diecezji śląsko-łódzkiej ",
        # SELECT * from v ORDER BY 'keyword' LIMIT 10;
        "keywordList": kl,
    }

    return render (
        request, "main/index.html", context
    )

def getUser(request, id = None):
    us, me, gotauth = None, None, None

    if id is not None:
        us = SearchUser.objects.all().get(id=id)
        me = False
        gotauth = True
    else:
        us = SearchUser.objects.all().get(id=1)
        me = True
        gotauth = False

    context = {
        "gotauth": gotauth,
        "me": me,
        "username": us.username,
        "description": us.userDescription,
        "friendList": us.friends,
    }

    return render (
        request, "main/user.html", context
    )

def getUserlist(request):
    us = SearchUser.objects.all().order_by('username')

    context = {
        "userList": us,
    }

    return render (
        request, "main/userlist.html", context
    )

def getResults(request):
    pass

# send request to NPWS webserver, save