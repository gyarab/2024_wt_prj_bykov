from django.shortcuts import render
from main.models import KeywordList, SearchUser, SearchUserAttributes

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

def getResults(request):
    pass

# send request to NPWS webserver, save