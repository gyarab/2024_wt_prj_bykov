from django.shortcuts import render
from main.models import KeywordList

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